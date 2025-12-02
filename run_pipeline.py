#!/usr/bin/env python3
"""
Script principal para executar o pipeline completo de verificação LLM-ReqVerif.

Permite executar etapas isoladamente ou o pipeline completo.

Uso:
    # Executar pipeline completo para uma tarefa
    python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude
    
    # Executar apenas geração de código (Etapa 1 + 2)
    python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --skip-verification
    
    # Executar apenas verificação
    python run_pipeline.py --task 1_fsm --skip-generation
    
    # Executar apenas uma etapa específica
    python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase 1
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path

def get_script_dir():
    """Retorna o diretório do script"""
    return os.path.dirname(os.path.abspath(__file__))

def run_generation(task, phase1, phase2, config=None, combination=None, 
                   api_keys=None, only_phase=None):
    """Executa a geração de código LLM"""
    script_dir = get_script_dir()
    generate_script = os.path.join(script_dir, "result", "generate_llm_code.py")
    
    cmd = [sys.executable, generate_script, "--task", task]
    
    if config and combination:
        cmd.extend(["--config", config, "--combination", combination])
    elif phase1 and phase2:
        cmd.extend(["--phase1", phase1, "--phase2", phase2])
    else:
        raise ValueError("Forneça --phase1 e --phase2 OU --config e --combination")
    
    if api_keys:
        cmd.extend(["--api-keys", api_keys])
    
    if only_phase:
        cmd.extend(["--only-phase", str(only_phase)])
    
    print(f"\n{'='*60}")
    print("ETAPA: Geração de Código LLM")
    print(f"{'='*60}")
    print(f"Comando: {' '.join(cmd)}")
    print(f"{'='*60}\n")
    
    result = subprocess.run(cmd)
    return result.returncode == 0

def run_verification(task=None, llm_dir=None, property_num=None, no_csv=False):
    """Executa a verificação ESBMC"""
    script_dir = get_script_dir()
    verify_script = os.path.join(script_dir, "result", "run_verification.py")
    
    # Muda para o diretório result para executar o script
    original_dir = os.getcwd()
    result_dir = os.path.join(script_dir, "result")
    
    cmd = [sys.executable, verify_script]
    
    if task:
        cmd.extend(["--task", task])
    if llm_dir:
        cmd.extend(["--llm-dir", llm_dir])
    if property_num:
        cmd.extend(["--property", str(property_num)])
    if no_csv:
        cmd.append("--no-csv")
    
    print(f"\n{'='*60}")
    print("ETAPA: Verificação ESBMC")
    print(f"{'='*60}")
    print(f"Comando: {' '.join(cmd)}")
    print(f"{'='*60}\n")
    
    try:
        os.chdir(result_dir)
        result = subprocess.run(cmd)
        return result.returncode == 0
    finally:
        os.chdir(original_dir)

def main():
    parser = argparse.ArgumentParser(
        description='Pipeline completo de verificação LLM-ReqVerif',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos de uso:

  # Pipeline completo para uma tarefa
  python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude

  # Apenas geração de código (pula verificação)
  python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --skip-verification

  # Apenas verificação (pula geração)
  python run_pipeline.py --task 1_fsm --skip-generation

  # Apenas Etapa 1 (especificação formal)
  python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase 1

  # Apenas Etapa 2 (código de verificação)
  python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase 2

  # Verificar apenas uma propriedade específica
  python run_pipeline.py --task 1_fsm --skip-generation --property 1

  # Usar arquivo de configuração
  python run_pipeline.py --task 1_fsm --config result/llm_config.json --combination ChatGPT_Claude
        """
    )
    
    # Argumentos para geração
    parser.add_argument('--task', help='Nome da tarefa (ex: 1_fsm, 0_triplex)')
    parser.add_argument('--phase1', help='Nome da LLM para Etapa 1 (ex: ChatGPT, Claude)')
    parser.add_argument('--phase2', help='Nome da LLM para Etapa 2 (ex: ChatGPT, Claude)')
    parser.add_argument('--config', help='Arquivo de configuração JSON')
    parser.add_argument('--combination', help='Nome da combinação do arquivo de configuração')
    parser.add_argument('--api-keys', help='Arquivo JSON com API keys (opcional)')
    parser.add_argument('--only-phase', type=int, choices=[1, 2],
                       help='Executar apenas uma etapa específica (1=Especificação Formal, 2=Código de Verificação)')
    
    # Argumentos para verificação
    parser.add_argument('--llm-dir', help='Caminho completo para diretório LLM_code específico')
    parser.add_argument('--property', type=int, help='Número da propriedade para verificar (ex: 1, 2, 3)')
    parser.add_argument('--no-csv', action='store_true', help='Não gerar arquivo CSV de resumo')
    
    # Controle de execução
    parser.add_argument('--skip-generation', action='store_true',
                       help='Pular etapa de geração de código LLM')
    parser.add_argument('--skip-verification', action='store_true',
                       help='Pular etapa de verificação ESBMC')
    
    args = parser.parse_args()
    
    # Validação de argumentos
    if not args.skip_generation:
        if not args.task:
            parser.error("--task é obrigatório quando --skip-generation não está definido")
        if not args.config and not (args.phase1 and args.phase2):
            parser.error("Forneça --phase1 e --phase2 OU --config e --combination")
        if args.config and not args.combination:
            parser.error("--combination é obrigatório quando --config é fornecido")
    
    if not args.skip_verification and not args.task and not args.llm_dir:
        parser.error("Forneça --task ou --llm-dir para executar verificação")
    
    success = True
    
    # Executa geração se não foi pulada
    if not args.skip_generation:
        success = run_generation(
            task=args.task,
            phase1=args.phase1,
            phase2=args.phase2,
            config=args.config,
            combination=args.combination,
            api_keys=args.api_keys,
            only_phase=args.only_phase
        )
        
        if not success:
            print("\n❌ Erro na geração de código. Abortando.")
            return 1
        
        # Se executou apenas uma fase específica, não continua para verificação
        if args.only_phase:
            print("\n✓ Etapa concluída. Use --only-phase para executar outras etapas.")
            return 0
    
    # Executa verificação se não foi pulada
    if not args.skip_verification:
        success = run_verification(
            task=args.task,
            llm_dir=args.llm_dir,
            property_num=args.property,
            no_csv=args.no_csv
        )
        
        if not success:
            print("\n❌ Erro na verificação. Verifique os logs.")
            return 1
    
    print(f"\n{'='*60}")
    print("✓ Pipeline concluído com sucesso!")
    print(f"{'='*60}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

