#!/usr/bin/env python3
import re
import subprocess
import os
import sys
import csv
from datetime import datetime
from collections import defaultdict

class PropertyVerifier:
    def __init__(self):
        # Get project root directory (assuming script is in result/)
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        include_dir = os.path.join(project_root, "include")
        
        self.include_paths = [
            "./",
            include_dir,  # Add project include directory for standard headers
            "/usr/local/MATLAB/R2024b/simulink/include",
            "/usr/local/MATLAB/R2024b/rtw/c/src",
            "/usr/local/MATLAB/R2024b/extern/include",
            "/usr/local/MATLAB/R2024b/simulink/include/sf_runtime"
        ]
        self.results_summary = defaultdict(dict)
        
    def find_esbmc_include_paths(self):
        """Find ESBMC standard include paths, especially for Windows"""
        include_paths = []
        
        if sys.platform == "win32":
            # Try to find ESBMC installation directory
            try:
                # Check common installation locations
                possible_paths = [
                    os.path.join(os.environ.get("ProgramFiles", ""), "esbmc", "include"),
                    os.path.join(os.environ.get("ProgramFiles(x86)", ""), "esbmc", "include"),
                    os.path.join(os.environ.get("LOCALAPPDATA", ""), "esbmc", "include"),
                    os.path.join(os.environ.get("APPDATA", ""), "esbmc", "include"),
                ]
                
                # Also try to find from PATH
                import shutil
                esbmc_path = shutil.which("esbmc")
                if esbmc_path:
                    esbmc_dir = os.path.dirname(esbmc_path)
                    possible_paths.extend([
                        os.path.join(esbmc_dir, "include"),
                        os.path.join(os.path.dirname(esbmc_dir), "include"),
                        os.path.join(os.path.dirname(esbmc_dir), "esbmclibc", "include"),
                    ])
                
                for path in possible_paths:
                    if path and os.path.exists(path):
                        include_paths.append(path)
                        break
            except Exception:
                pass
            
            # If no specific path found, ESBMC should use esbmclibc automatically
            # But we can try adding common system include paths as fallback
            if not include_paths:
                # Try to use --idirafter with common Windows paths
                # ESBMC on Windows should handle standard headers via esbmclibc
                pass
        
        return include_paths
        
    def extract_properties(self, file_path):
        """Extract all VERIFY_PROPERTY definitions from file"""
        properties = set()
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                matches = re.findall(r'#ifdef\s+(VERIFY_PROPERTY_\d+)', content)
                for match in matches:
                    prop_num = int(match.split('_')[-1])
                    properties.add(prop_num)
        except Exception as e:
            print(f"Error reading file {file_path}: {e}")
        return sorted(list(properties))

    def run_verification(self, working_dir, property_num):
        """Run ESBMC verification for a specific property"""
        c_files = [f for f in os.listdir(working_dir) if f.endswith('.c')]
        if not c_files:
            return "ERROR", "No .c files found"

        original_dir = os.getcwd()
        os.chdir(working_dir)

        try:
            # Construct include paths arguments
            include_args = []
            for path in self.include_paths:
                # Skip Unix-specific paths on Windows
                if sys.platform == "win32" and path.startswith("/usr/"):
                    continue
                # Add path if it exists or is a relative path (like "./")
                if os.path.exists(path) or not path.startswith("/"):
                    include_args.extend(["-I", path])
                # On Windows, also try to add include directory even if path doesn't start with /
                elif sys.platform == "win32" and not path.startswith("/"):
                    include_args.extend(["-I", path])
            
            # Add ESBMC standard include paths for Windows
            if sys.platform == "win32":
                esbmc_includes = self.find_esbmc_include_paths()
                for path in esbmc_includes:
                    include_args.extend(["-I", path])
            
            # Build ESBMC command
            cmd = ["esbmc"] + c_files + include_args + [
                "--k-induction",
                f"-DVERIFY_PROPERTY_{property_num}"
            ]
            
            # On Windows, ESBMC uses esbmclibc for standard headers
            # If headers are not found, try to locate esbmclibc installation
            if sys.platform == "win32":
                # Try to find esbmclibc include directory
                try:
                    import shutil
                    esbmc_path = shutil.which("esbmc")
                    if esbmc_path:
                        esbmc_dir = os.path.dirname(esbmc_path)
                        # Common locations for esbmclibc headers
                        possible_libc_paths = [
                            os.path.join(esbmc_dir, "..", "esbmclibc", "include"),
                            os.path.join(esbmc_dir, "..", "include"),
                            os.path.join(os.path.dirname(esbmc_dir), "esbmclibc", "include"),
                            os.path.join(os.path.dirname(esbmc_dir), "include"),
                        ]
                        for libc_path in possible_libc_paths:
                            libc_path = os.path.normpath(libc_path)
                            if os.path.exists(libc_path) and os.path.exists(os.path.join(libc_path, "stdio.h")):
                                cmd.extend(["--idirafter", libc_path])
                                break
                except Exception:
                    pass
            
            # Add timeout and memlimit options only on non-Windows systems
            # (ESBMC timeout and memlimit are not implemented on Windows)
            if sys.platform != "win32":
                cmd.extend(["--timeout", "300", "--memlimit", "8g"])

            result = subprocess.run(cmd, 
                                  capture_output=True, 
                                  text=True,
                                  timeout=300)
            output = result.stdout + result.stderr
            
            if "VERIFICATION SUCCESSFUL" in output:
                status = "SUCCESSFUL"
            elif "VERIFICATION FAILED" in output:
                status = "FAILED"
            else:
                status = "UNKNOWN"
        except subprocess.TimeoutExpired:
            status, output = "TIMEOUT", "Verification timed out"
        except Exception as e:
            status, output = "ERROR", str(e)
        finally:
            os.chdir(original_dir)
            
        return status, output

    def process_directory(self, directory, results_dir, task_name):
        """Process verification for a specific directory"""
        print(f"\nProcessing directory: {directory}")
        
        properties_found = False
        source_files = [f for f in os.listdir(directory) if f.endswith('.c') or f.endswith('.h')]
        dir_name = os.path.basename(directory)
        
        for source_file in source_files:
            file_path = os.path.join(directory, source_file)
            properties = self.extract_properties(file_path)
            
            if properties:
                properties_found = True
                print(f"Found properties in {source_file}: {properties}")
                
                # Create summary entry for this attempt
                attempt_results = {}
                
                log_path = os.path.join(results_dir, f"{dir_name}_summary.log")
                with open(log_path, 'w') as log:
                    log.write(f"Verification Summary - {datetime.now()}\n")
                    log.write(f"Directory: {directory}\n")
                    log.write(f"Source file: {source_file}\n")
                    log.write("=" * 50 + "\n\n")
                    
                    for prop_num in properties:
                        print(f"  Verifying Property {prop_num}...")
                        log.write(f"Property {prop_num}\n")
                        log.write("-" * 20 + "\n")
                        
                        status, output = self.run_verification(directory, prop_num)
                        attempt_results[prop_num] = status
                        
                        output_file = os.path.join(results_dir, f"{dir_name}_property_{prop_num}_result.txt")
                        with open(output_file, 'w') as f:
                            f.write(output)
                        
                        result_line = f"Status: {status}"
                        print(f"    {result_line}")
                        log.write(f"{result_line}\n\n")
                
                # Store results in summary dictionary
                self.results_summary[task_name][dir_name] = attempt_results
                break
        
        return properties_found

def process_llm_directory(verifier, llm_dir):
    """Process all subdirectories in an LLM directory"""
    directories_processed = 0
    
    # Get task name from parent directory
    task_name = os.path.basename(os.path.dirname(llm_dir))
    
    # Create verification_results in parent directory of LLM_code
    parent_dir = os.path.dirname(llm_dir)
    results_dir = os.path.join(parent_dir, "verification_results")
    if not os.path.exists(results_dir):
        os.makedirs(results_dir)
    
    # Process all subdirectories in the LLM directory
    for item in os.listdir(llm_dir):
        subdir_path = os.path.join(llm_dir, item)
        if os.path.isdir(subdir_path):
            print(f"Checking subdirectory: {subdir_path}")
            if verifier.process_directory(subdir_path, results_dir, task_name):
                directories_processed += 1
    
    return directories_processed

def find_llm_dirs():
    """Find all directories containing 'LLM_code' in their name
    
    Suporta tanto a estrutura antiga (ChatGPT_code, Claude_code) quanto
    a nova estrutura (ChatGPT_Claude, Claude_ChatGPT, etc.)
    """
    llm_dirs = []
    for root, dirs, _ in os.walk('.'):
        if 'LLM_code' in dirs:
            llm_code_path = os.path.join(root, 'LLM_code')
            # Procura subdiretórios dentro de LLM_code
            if os.path.exists(llm_code_path):
                has_valid_subdir = False
                try:
                    for item in os.listdir(llm_code_path):
                        subdir_path = os.path.join(llm_code_path, item)
                        if os.path.isdir(subdir_path):
                            # Verifica se contém arquivos .c (código de verificação)
                            try:
                                has_c_files = any(f.endswith('.c') for f in os.listdir(subdir_path) 
                                                if os.path.isfile(os.path.join(subdir_path, f)))
                                if has_c_files:
                                    has_valid_subdir = True
                                    break
                            except (PermissionError, OSError):
                                continue
                except (PermissionError, OSError):
                    continue
                if has_valid_subdir:
                    llm_dirs.append(llm_code_path)
    return llm_dirs

def create_summary_csv(results_summary):
    """Create a CSV summary of all verification results"""
    # Get all unique property numbers across all tasks and attempts
    all_properties = set()
    for task_results in results_summary.values():
        for attempt_results in task_results.values():
            all_properties.update(attempt_results.keys())
    all_properties = sorted(list(all_properties))
    
    # Prepare CSV headers
    headers = ['Task', 'Attempt'] + [f'Property_{num}' for num in all_properties]
    
    # Create CSV file
    with open('verification_summary.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(headers)
        
        # Write results for each task and attempt
        for task_name, task_results in sorted(results_summary.items()):
            for attempt_name, attempt_results in sorted(task_results.items()):
                row = [task_name, attempt_name]
                for prop_num in all_properties:
                    row.append(attempt_results.get(prop_num, 'N/A'))
                writer.writerow(row)
    
    print(f"\nSummary CSV created: verification_summary.csv")

def main():
    llm_dirs = find_llm_dirs()
    
    if not llm_dirs:
        print("No directories containing 'LLM' found.")
        return
    
    print(f"Found {len(llm_dirs)} LLM directories:")
    for dir in llm_dirs:
        print(f"  {dir}")
    
    verifier = PropertyVerifier()
    total_directories_processed = 0
    
    for llm_dir in llm_dirs:
        print(f"\nProcessing LLM directory: {llm_dir}")
        directories_processed = process_llm_directory(verifier, llm_dir)
        total_directories_processed += directories_processed
    
    if total_directories_processed == 0:
        print("\nNo files with VERIFY_PROPERTY definitions found in any directory.")
    else:
        print(f"\nVerification complete. Processed {total_directories_processed} directories.")
        # Create summary CSV
        create_summary_csv(verifier.results_summary)

if __name__ == "__main__":
    main()
