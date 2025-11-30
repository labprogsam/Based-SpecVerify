#!/usr/bin/env python3
"""
Script para limpar arquivos ert_main.c já gerados, removendo texto explicativo e markdown.
"""

import os
import re
import argparse
from pathlib import Path

def clean_code_file(file_path: str) -> bool:
    """Limpa um arquivo ert_main.c removendo markdown e texto explicativo
    
    Args:
        file_path: Caminho para o arquivo a ser limpo
        
    Returns:
        True se o arquivo foi modificado, False caso contrário
    """
    if not os.path.exists(file_path):
        return False
    
    with open(file_path, 'r', encoding='utf-8') as f:
        original_code = f.read()
    
    # Verifica se precisa limpar (tem texto explicativo ou markdown)
    if not ('```' in original_code or original_code.strip().startswith('Looking') or 
            original_code.strip().startswith('Based on')):
        return False  # Já está limpo
    
    lines = original_code.split('\n')
    cleaned_lines = []
    in_code_block = False
    skip_until_code = False
    
    for line in lines:
        # Detecta início de bloco de código markdown
        if line.strip().startswith('```'):
            in_code_block = not in_code_block
            continue
        
        # Se estamos dentro de um bloco de código, adiciona a linha
        if in_code_block:
            cleaned_lines.append(line)
            continue
        
        # Se encontrou texto explicativo antes do código, pula até encontrar código válido
        if not skip_until_code:
            # Verifica se a linha parece ser código C válido
            stripped = line.strip()
            if stripped and not stripped.startswith('Looking') and not stripped.startswith('Based on'):
                # Verifica se parece código C
                if (stripped.startswith('#') or 
                    stripped.startswith('/*') or 
                    stripped.startswith('//') or
                    stripped.startswith('*') or
                    any(keyword in stripped for keyword in ['#include', 'int ', 'void ', 'double ', '_Bool ', 'extern ', 'static ', 'const ', '#define', '#ifdef', '#endif'])):
                    skip_until_code = True
                    cleaned_lines.append(line)
            # Se não parece código, pula
            continue
        
        # Após encontrar código válido, adiciona todas as linhas
        cleaned_lines.append(line)
    
    # Se não encontrou código válido, tenta extrair de blocos markdown
    if not cleaned_lines or not any('#include' in line for line in cleaned_lines):
        # Tenta encontrar código entre ```c e ```
        code_block_pattern = r'```c\s*\n(.*?)```'
        matches = re.findall(code_block_pattern, original_code, re.DOTALL)
        if matches:
            cleaned_lines = matches[0].split('\n')
    
    # Remove linhas vazias no início e fim
    while cleaned_lines and not cleaned_lines[0].strip():
        cleaned_lines.pop(0)
    while cleaned_lines and not cleaned_lines[-1].strip():
        cleaned_lines.pop()
    
    cleaned_code = '\n'.join(cleaned_lines)
    
    # Só salva se mudou algo
    if cleaned_code != original_code:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(cleaned_code)
        return True
    
    return False

def find_all_ert_main_files(root_dir: str = '.'):
    """Encontra todos os arquivos ert_main.c recursivamente"""
    ert_files = []
    for root, dirs, files in os.walk(root_dir):
        if 'ert_main.c' in files:
            ert_files.append(os.path.join(root, 'ert_main.c'))
    return ert_files

def main():
    parser = argparse.ArgumentParser(
        description='Limpa arquivos ert_main.c removendo markdown e texto explicativo',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos de uso:
  # Limpar todos os arquivos ert_main.c no diretório atual e subdiretórios
  python clean_generated_files.py
  
  # Limpar arquivo específico
  python clean_generated_files.py --file result/0_triplex/LLM_code/ChatGPT_Claude/ert_main.c
  
  # Limpar apenas uma task específica
  python clean_generated_files.py --task 0_triplex
        """
    )
    parser.add_argument('--file', help='Arquivo específico para limpar')
    parser.add_argument('--task', help='Task específica para limpar (ex: 0_triplex)')
    parser.add_argument('--dry-run', action='store_true', help='Apenas mostra o que seria limpo, sem modificar')
    
    args = parser.parse_args()
    
    files_to_clean = []
    
    if args.file:
        if os.path.exists(args.file):
            files_to_clean = [args.file]
        else:
            print(f"❌ Arquivo não encontrado: {args.file}")
            return 1
    elif args.task:
        task_dir = os.path.join('.', args.task, 'LLM_code')
        if os.path.exists(task_dir):
            files_to_clean = find_all_ert_main_files(task_dir)
        else:
            print(f"❌ Diretório da task não encontrado: {task_dir}")
            return 1
    else:
        # Limpa todos os arquivos encontrados
        files_to_clean = find_all_ert_main_files('.')
    
    if not files_to_clean:
        print("Nenhum arquivo ert_main.c encontrado.")
        return 0
    
    print(f"Encontrados {len(files_to_clean)} arquivo(s) ert_main.c")
    if args.dry_run:
        print("⚠️  Modo dry-run: nenhum arquivo será modificado\n")
    
    cleaned_count = 0
    for file_path in files_to_clean:
        if args.dry_run:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            if '```' in content or content.strip().startswith('Looking') or content.strip().startswith('Based on'):
                print(f"  [SERIA LIMPO] {file_path}")
                cleaned_count += 1
            else:
                print(f"  [OK] {file_path}")
        else:
            if clean_code_file(file_path):
                print(f"✓ Limpo: {file_path}")
                cleaned_count += 1
            else:
                print(f"  OK: {file_path}")
    
    print(f"\n{'='*60}")
    if args.dry_run:
        print(f"Total que seria limpo: {cleaned_count}/{len(files_to_clean)}")
    else:
        print(f"Total limpo: {cleaned_count}/{len(files_to_clean)}")
    print(f"{'='*60}")
    
    return 0

if __name__ == "__main__":
    exit(main())

