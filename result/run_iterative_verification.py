#!/usr/bin/env python3
"""
Script para executar ciclo iterativo de verificação e melhoria de código.

Este script:
1. Gera código de verificação inicial
2. Executa verificação
3. Extrai erros encontrados
4. Re-executa geração de código incluindo os erros
5. Repete até não haver mais erros ou atingir limite de iterações

Uso:
    python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --property 4
    python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --max-iterations 5
"""

import os
import sys
import json
import argparse
import shutil
from pathlib import Path

# Adiciona o diretório atual ao path para importar módulos
script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, script_dir)

from generate_llm_code import LLMGenerator, LLMConfig, create_llm_config_from_dict, load_config, find_c_code_files
from run_verification import PropertyVerifier

def extract_violated_properties(verification_result_file: str) -> list:
    """Extrai informações sobre propriedades violadas do arquivo de resultado"""
    from run_verification import PropertyVerifier
    verifier = PropertyVerifier()
    return verifier.extract_violated_properties(verification_result_file)

def run_iterative_verification(task: str, phase1: str, phase2: str, 
                                max_iterations: int = 3, property_num: int = None,
                                config_file: str = None, combination: str = None,
                                api_keys_file: str = None, only_phase2: bool = False):
    """Executa ciclo iterativo de verificação e melhoria
    
    Args:
        task: Nome da tarefa (ex: 1_fsm, 0_triplex)
        phase1: Nome da LLM para Etapa 1
        phase2: Nome da LLM para Etapa 2
        max_iterations: Número máximo de iterações
        property_num: Número da propriedade específica para verificar (None = todas)
        config_file: Arquivo de configuração JSON (opcional)
        combination: Nome da combinação no arquivo de configuração (opcional)
        api_keys_file: Arquivo com API keys (opcional)
        only_phase2: Se True, pula a Etapa 1 e usa especificação existente
    """
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    task_dir = os.path.join(script_dir, task)
    
    if not os.path.exists(task_dir):
        raise FileNotFoundError(f"Diretório da tarefa não encontrado: {task_dir}")
    
    # Carrega API keys se fornecido
    api_keys = {}
    if api_keys_file:
        if os.path.exists(api_keys_file):
            with open(api_keys_file, 'r') as f:
                api_keys = json.load(f)
        else:
            print(f"⚠️  Arquivo de API keys não encontrado: {api_keys_file}")
    else:
        # Tenta carregar automaticamente api_keys.json do diretório do script
        api_keys_path = os.path.join(script_dir, "api_keys.json")
        if os.path.exists(api_keys_path):
            with open(api_keys_path, 'r') as f:
                api_keys = json.load(f)
            print(f"✓ API keys carregadas de: {api_keys_path}")
    
    # Determina configurações das LLMs
    if config_file and combination:
        # Usa arquivo de configuração
        config = load_config(config_file)
        combination_config = None
        for comb in config.get('combinations', []):
            if comb['name'] == combination:
                combination_config = comb
                break
        
        if not combination_config:
            raise ValueError(f"Combinação '{combination}' não encontrada no arquivo de configuração")
        
        phase1_config = create_llm_config_from_dict(combination_config['phase1'], api_keys)
        phase2_config = create_llm_config_from_dict(combination_config['phase2'], api_keys)
        output_name = combination
    elif phase1 and phase2:
        # Usa argumentos da linha de comando
        default_configs_phase1 = {
            'ChatGPT': {
                'provider': 'openai',
                'model': 'gpt-5',
                'api_key_env': 'OPENAI_API_KEY'
            },
            'Claude': {
                'provider': 'anthropic',
                'model': 'claude-opus-4-1-20250805',
                'api_key_env': 'ANTHROPIC_API_KEY'
            },
            'Llama': {
                'provider': 'ollama',
                'model': 'llama3.1',
                'api_key_env': ''
            }
        }
        
        default_configs_phase2 = {
            'ChatGPT': {
                'provider': 'openai',
                'model': 'gpt-5',
                'api_key_env': 'OPENAI_API_KEY'
            },
            'Claude': {
                'provider': 'anthropic',
                'model': 'claude-opus-4-1-20250805',
                'api_key_env': 'ANTHROPIC_API_KEY'
            },
            'Llama': {
                'provider': 'ollama',
                'model': 'llama3.1',
                'api_key_env': ''
            }
        }
        
        phase1_dict = default_configs_phase1.get(phase1, {})
        phase2_dict = default_configs_phase2.get(phase2, {})
        
        phase1_config = create_llm_config_from_dict({
            'name': phase1,
            **phase1_dict
        }, api_keys)
        phase2_config = create_llm_config_from_dict({
            'name': phase2,
            **phase2_dict
        }, api_keys)
        output_name = f"{phase1}_{phase2}"
    else:
        raise ValueError("Forneça --phase1 e --phase2 OU --config e --combination")
    
    # Cria o gerador
    generator = LLMGenerator(phase1_config, phase2_config)
    
    # Encontra arquivos necessários
    reqs_file = os.path.join(task_dir, f"{task}_reqs.txt")
    if not os.path.exists(reqs_file):
        # Tenta variações do nome
        for file in os.listdir(task_dir):
            if file.endswith('_reqs.txt'):
                reqs_file = os.path.join(task_dir, file)
                break
        else:
            raise FileNotFoundError(f"Arquivo de requisitos não encontrado em {task_dir}")
    
    c_code_file, h_file = find_c_code_files(task_dir)
    header_files = [h_file] if h_file else []
    
    # Define diretório de saída
    output_dir = os.path.join(task_dir, "LLM_code", output_name)
    results_dir = os.path.join(task_dir, "verification_results")
    os.makedirs(results_dir, exist_ok=True)
    
    print(f"\n{'='*60}")
    print(f"Ciclo Iterativo de Verificação")
    print(f"{'='*60}")
    print(f"Tarefa: {task}")
    print(f"Etapa 1 (Especificação Formal): {phase1_config.name}")
    print(f"Etapa 2 (Código de Verificação): {phase2_config.name}")
    print(f"Diretório de saída: {output_dir}")
    print(f"Máximo de iterações: {max_iterations}")
    if property_num:
        print(f"Propriedade específica: {property_num}")
    print(f"{'='*60}\n")
    
    # Etapa 1: Gera especificação formal (apenas na primeira iteração se não for only_phase2)
    formal_spec_path = os.path.join(output_dir, "formal_specification.txt")
    
    if not only_phase2:
        if not os.path.exists(formal_spec_path) or not os.path.getsize(formal_spec_path):
            print("Gerando especificação formal (Etapa 1)...")
            formal_spec_path = generator.generate_formal_specification(
                c_code_path=c_code_file,
                reqs_path=reqs_file,
                output_dir=output_dir
            )
            
            # Copia arquivos .c e .h necessários
            if c_code_file:
                shutil.copy(c_code_file, output_dir)
                print(f"✓ Arquivo .c copiado para: {output_dir}")
            if h_file:
                shutil.copy(h_file, output_dir)
                print(f"✓ Arquivo .h copiado para: {output_dir}")
        else:
            print(f"✓ Usando especificação formal existente: {formal_spec_path}")
    else:
        if not os.path.exists(formal_spec_path):
            raise FileNotFoundError(
                f"Especificação formal não encontrada em {formal_spec_path}. "
                f"Execute a Etapa 1 primeiro ou remova --only-phase2."
            )
        print(f"✓ Usando especificação formal existente: {formal_spec_path}")
    
    # Inicializa verifier
    verifier = PropertyVerifier()
    
    # Extrai propriedades - será atualizado após primeira geração de código
    properties_to_check = []
    
    # Se property_num foi especificado, usa apenas ele
    if property_num:
        properties_to_check = [property_num]
        print(f"Propriedade específica a verificar: {property_num}\n")
    else:
        # Tenta extrair propriedades do arquivo ert_main.c se existir
        ert_main_path = os.path.join(output_dir, "ert_main.c")
        if os.path.exists(ert_main_path):
            properties_to_check = verifier.extract_properties(ert_main_path)
        
        # Se não encontrou, tenta extrair de qualquer arquivo .c no diretório
        if not properties_to_check:
            if os.path.exists(output_dir):
                for file in os.listdir(output_dir):
                    if file.endswith('.c') and file != 'ert_main.c':
                        file_path = os.path.join(output_dir, file)
                        properties_to_check = verifier.extract_properties(file_path)
                        if properties_to_check:
                            break
        
        if not properties_to_check:
            print("⚠️  Nenhuma propriedade encontrada. Será extraída após primeira geração de código.\n")
        else:
            print(f"Propriedades a verificar: {properties_to_check}\n")
    
    previous_errors = []
    iteration_results = []
    
    for iteration in range(1, max_iterations + 1):
        print(f"\n{'='*60}")
        print(f"Iteração {iteration}/{max_iterations}")
        print(f"{'='*60}")
        
        # Etapa 2: Gera código de verificação com erros anteriores
        print(f"\nGerando código de verificação (Etapa 2)...")
        if previous_errors:
            print(f"  Incluindo {len(previous_errors)} erro(s) de iterações anteriores")
        
        ert_main_path = generator.generate_verification_code(
            formal_spec_path=formal_spec_path,
            c_code_path=c_code_file,
            output_dir=output_dir,
            header_files=header_files,
            previous_errors=previous_errors
        )
        
        # Atualiza lista de propriedades se ainda não foi definida
        if not properties_to_check:
            ert_main_path = os.path.join(output_dir, "ert_main.c")
            if os.path.exists(ert_main_path):
                properties_to_check = verifier.extract_properties(ert_main_path)
                if properties_to_check:
                    print(f"  Propriedades encontradas: {properties_to_check}")
                else:
                    print("  ⚠️  Nenhuma propriedade encontrada no código gerado.")
                    break
        
        # Executa verificação para cada propriedade
        all_passed = True
        current_iteration_errors = []
        iteration_result = {
            'iteration': iteration,
            'properties': {}
        }
        
        for prop_num in properties_to_check:
            print(f"\n  Verificando Property {prop_num}...")
            status, output = verifier.run_verification(output_dir, prop_num)
            
            # Salva resultado
            result_file = os.path.join(results_dir, 
                f"{output_name}_property_{prop_num}_iter_{iteration}_result.txt")
            with open(result_file, 'w', encoding='utf-8') as f:
                f.write(output)
            
            iteration_result['properties'][prop_num] = {
                'status': status,
                'result_file': result_file
            }
            
            if status == "FAILED":
                all_passed = False
                # Extrai erros
                errors = extract_violated_properties(result_file)
                current_iteration_errors.extend(errors)
                print(f"    ✗ Property {prop_num} FALHOU - {len(errors)} erro(s) encontrado(s)")
                for i, error in enumerate(errors, 1):
                    print(f"      Erro {i}: {error.get('message', 'N/A')[:80]}...")
            elif status == "SUCCESSFUL":
                print(f"    ✓ Property {prop_num} PASSOU")
            else:
                print(f"    ? Property {prop_num} {status}")
        
        iteration_results.append(iteration_result)
        
        # Se todas as propriedades passaram, para o ciclo
        if all_passed:
            print(f"\n{'='*60}")
            print("✓ Todas as propriedades passaram na verificação!")
            print(f"{'='*60}")
            break
        
        # Prepara erros para próxima iteração
        previous_errors = current_iteration_errors
        if previous_errors:
            print(f"\n  {len(previous_errors)} erro(s) encontrado(s). Preparando para próxima iteração...")
        else:
            print(f"\n  ⚠️  Nenhum erro extraído, mas verificação falhou. Continuando...")
    
    else:
        print(f"\n{'='*60}")
        print(f"⚠️  Limite de {max_iterations} iterações atingido.")
        print(f"{'='*60}")
    
    # Resumo final
    print(f"\n{'='*60}")
    print("Resumo das Iterações")
    print(f"{'='*60}")
    for iter_result in iteration_results:
        print(f"\nIteração {iter_result['iteration']}:")
        for prop_num, prop_result in iter_result['properties'].items():
            status = prop_result['status']
            status_symbol = "✓" if status == "SUCCESSFUL" else "✗" if status == "FAILED" else "?"
            print(f"  {status_symbol} Property {prop_num}: {status}")
    
    print(f"\n{'='*60}")
    print("Ciclo iterativo concluído!")
    print(f"{'='*60}")

def main():
    parser = argparse.ArgumentParser(
        description='Executa ciclo iterativo de verificação e melhoria de código',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos de uso:
  # Executar ciclo iterativo completo
  python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --property 4
  
  # Executar com limite de iterações
  python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --max-iterations 5
  
  # Executar apenas Etapa 2 (usar especificação existente)
  python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase2
  
  # Usar arquivo de configuração
  python run_iterative_verification.py --task 1_fsm --config llm_config.json --combination ChatGPT_Claude
        """
    )
    parser.add_argument('--task', required=True, help='Nome da tarefa (ex: 1_fsm, 0_triplex)')
    parser.add_argument('--phase1', help='Nome da LLM para Etapa 1 (ex: ChatGPT, Claude)')
    parser.add_argument('--phase2', help='Nome da LLM para Etapa 2 (ex: ChatGPT, Claude)')
    parser.add_argument('--config', help='Arquivo de configuração JSON')
    parser.add_argument('--combination', help='Nome da combinação do arquivo de configuração')
    parser.add_argument('--api-keys', help='Arquivo JSON com API keys (opcional)')
    parser.add_argument('--property', type=int, help='Número da propriedade específica para verificar')
    parser.add_argument('--max-iterations', type=int, default=3, 
                       help='Número máximo de iterações (padrão: 3)')
    parser.add_argument('--only-phase2', action='store_true',
                       help='Executar apenas Etapa 2 (usar especificação existente)')
    
    args = parser.parse_args()
    
    try:
        run_iterative_verification(
            task=args.task,
            phase1=args.phase1,
            phase2=args.phase2,
            max_iterations=args.max_iterations,
            property_num=args.property,
            config_file=args.config,
            combination=args.combination,
            api_keys_file=args.api_keys,
            only_phase2=args.only_phase2
        )
    except Exception as e:
        print(f"\n❌ Erro: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()

