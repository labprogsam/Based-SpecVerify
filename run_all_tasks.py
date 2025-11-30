#!/usr/bin/env python3
"""
Script para executar todas as tasks usando as duas fases do LLM.
"""

import os
import sys
import subprocess
from pathlib import Path

# Lista de todas as tasks disponíveis
TASKS = [
    "0_triplex",
    "1_fsm",
    "2_tustin",
    "3_regulators",  # ou "3_regs"
    "4_nlguide",
    "5_nn",
    "6_eb",
    "8_swim",
    "9_euler"  # ou "9_euler321_I2B"
]

def run_pipeline_for_task(task, phase1, phase2, skip_verification=True):
    """
    Executa o pipeline para uma task específica.
    
    Args:
        task: Nome da task
        phase1: LLM para Fase 1
        phase2: LLM para Fase 2
        skip_verification: Se True, pula a verificação (apenas gera código)
    """
    cmd = [
        sys.executable,
        "run_pipeline.py",
        "--task", task,
        "--phase1", phase1,
        "--phase2", phase2
    ]
    
    if skip_verification:
        cmd.append("--skip-verification")
    
    print(f"\n{'='*80}")
    print(f"Executando: {task}")
    print(f"Fase 1: {phase1} | Fase 2: {phase2}")
    print(f"{'='*80}")
    print(f"Comando: {' '.join(cmd)}")
    print(f"{'='*80}\n")
    
    result = subprocess.run(cmd)
    return result.returncode == 0

def main():
    """Executa todas as tasks"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Executa todas as tasks usando as duas fases do LLM',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  # Executar todas as tasks com ChatGPT (Fase 1) e Claude (Fase 2)
  python run_all_tasks.py --phase1 ChatGPT --phase2 Claude
  
  # Executar todas as tasks e também executar verificação
  python run_all_tasks.py --phase1 ChatGPT --phase2 Claude --run-verification
  
  # Executar apenas tasks específicas
  python run_all_tasks.py --phase1 ChatGPT --phase2 Claude --tasks 1_fsm 2_tustin
  
  # Retomar execução a partir de uma task específica
  python run_all_tasks.py --phase1 ChatGPT --phase2 Claude --start-from 4_nlguide
        """
    )
    
    parser.add_argument('--phase1', required=True,
                       help='LLM para Fase 1 (ex: ChatGPT, Claude, Claude Sonnet 4.5)')
    parser.add_argument('--phase2', required=True,
                       help='LLM para Fase 2 (ex: ChatGPT, Claude, Claude Sonnet 4.5)')
    parser.add_argument('--tasks', nargs='+',
                       help='Lista de tasks específicas para executar (opcional)')
    parser.add_argument('--run-verification', action='store_true',
                       help='Também executar verificação após gerar código')
    parser.add_argument('--start-from', 
                       help='Começar a partir de uma task específica (útil para retomar execução)')
    
    args = parser.parse_args()
    
    # Determina quais tasks executar
    tasks_to_run = args.tasks if args.tasks else TASKS
    
    # Se --start-from foi fornecido, filtra tasks
    if args.start_from:
        try:
            start_index = tasks_to_run.index(args.start_from)
            tasks_to_run = tasks_to_run[start_index:]
            print(f"⚠️  Começando a partir de: {args.start_from}")
        except ValueError:
            print(f"⚠️  Task '{args.start_from}' não encontrada. Executando todas.")
    
    print(f"\n{'='*80}")
    print("EXECUÇÃO DE TODAS AS TASKS")
    print(f"{'='*80}")
    print(f"Total de tasks: {len(tasks_to_run)}")
    print(f"Fase 1 (Especificação Formal): {args.phase1}")
    print(f"Fase 2 (Código de Verificação): {args.phase2}")
    print(f"Executar verificação: {'Sim' if args.run_verification else 'Não'}")
    print(f"{'='*80}\n")
    
    # Estatísticas
    successful_tasks = []
    failed_tasks = []
    
    # Executa cada task
    for i, task in enumerate(tasks_to_run, 1):
        print(f"\n[{i}/{len(tasks_to_run)}] Processando task: {task}")
        
        success = run_pipeline_for_task(
            task=task,
            phase1=args.phase1,
            phase2=args.phase2,
            skip_verification=not args.run_verification
        )
        
        if success:
            successful_tasks.append(task)
            print(f"✓ Task {task} concluída com sucesso!")
        else:
            failed_tasks.append(task)
            print(f"❌ Task {task} falhou!")
            
            # Pergunta se deve continuar
            response = input("\nDeseja continuar com as próximas tasks? (s/n): ")
            if response.lower() != 's':
                print("Execução interrompida pelo usuário.")
                break
    
    # Resumo final
    print(f"\n{'='*80}")
    print("RESUMO DA EXECUÇÃO")
    print(f"{'='*80}")
    print(f"Tasks executadas com sucesso: {len(successful_tasks)}/{len(tasks_to_run)}")
    if successful_tasks:
        print("\n✓ Tasks bem-sucedidas:")
        for task in successful_tasks:
            print(f"  - {task}")
    
    if failed_tasks:
        print("\n❌ Tasks que falharam:")
        for task in failed_tasks:
            print(f"  - {task}")
    
    print(f"\n{'='*80}")
    
    # Se foi solicitado, executa verificação para todas as tasks
    if args.run_verification and successful_tasks:
        print("\n" + "="*80)
        print("EXECUTANDO VERIFICAÇÃO PARA TODAS AS TASKS")
        print("="*80)
        
        original_dir = os.getcwd()
        result_dir = os.path.join(original_dir, "result")
        
        try:
            os.chdir(result_dir)
            verify_cmd = [sys.executable, "run_verification.py"]
            print(f"Comando: {' '.join(verify_cmd)}")
            result = subprocess.run(verify_cmd)
            
            if result.returncode == 0:
                print("\n✓ Verificação concluída com sucesso!")
            else:
                print("\n❌ Erro na verificação.")
        finally:
            os.chdir(original_dir)
    
    return 0 if not failed_tasks else 1

if __name__ == "__main__":
    sys.exit(main())

