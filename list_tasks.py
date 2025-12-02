#!/usr/bin/env python3
"""
Script para listar todas as tarefas disponíveis no projeto.

Uso:
    python list_tasks.py
"""

import os
from pathlib import Path

def get_project_root():
    """Retorna o diretório raiz do projeto"""
    return Path(__file__).parent

def list_available_tasks():
    """Lista todas as tarefas disponíveis"""
    project_root = get_project_root()
    result_dir = project_root / "result"
    
    if not result_dir.exists():
        print("❌ Diretório 'result' não encontrado.")
        return []
    
    tasks = []
    for item in sorted(result_dir.iterdir()):
        if item.is_dir() and item.name[0].isdigit():
            # Verifica se tem arquivo de requisitos
            reqs_file = None
            for req_file in item.glob("*_reqs.txt"):
                reqs_file = req_file.name
                break
            
            # Verifica se tem código LLM
            llm_code_dir = item / "LLM_code"
            has_llm_code = llm_code_dir.exists() and any(llm_code_dir.iterdir())
            
            # Conta quantos códigos LLM existem
            llm_count = 0
            if has_llm_code:
                llm_count = len([d for d in llm_code_dir.iterdir() if d.is_dir()])
            
            tasks.append({
                'name': item.name,
                'path': item,
                'reqs_file': reqs_file,
                'has_llm_code': has_llm_code,
                'llm_count': llm_count
            })
    
    return tasks

def main():
    print("=" * 60)
    print("Tarefas Disponíveis no Projeto LLM-ReqVerif")
    print("=" * 60)
    print()
    
    tasks = list_available_tasks()
    
    if not tasks:
        print("Nenhuma tarefa encontrada.")
        return
    
    print(f"Total de tarefas encontradas: {len(tasks)}\n")
    
    for i, task in enumerate(tasks, 1):
        print(f"{i}. {task['name']}")
        print(f"   📁 Caminho: {task['path']}")
        if task['reqs_file']:
            print(f"   📄 Requisitos: {task['reqs_file']}")
        else:
            print(f"   ⚠️  Arquivo de requisitos não encontrado")
        
        if task['has_llm_code']:
            print(f"   ✅ Código LLM: {task['llm_count']} implementação(ões)")
        else:
            print(f"   ❌ Código LLM: Não gerado")
        print()
    
    print("=" * 60)
    print("Exemplos de uso:")
    print("=" * 60)
    print()
    
    if tasks:
        example_task = tasks[0]['name']
        print(f"# Executar pipeline completo para {example_task}")
        print(f"python run_pipeline.py --task {example_task} --phase1 ChatGPT --phase2 Claude")
        print()
        print(f"# Apenas verificação para {example_task}")
        print(f"python run_pipeline.py --task {example_task} --skip-generation")
        print()
        print("# Listar todas as tarefas com código LLM gerado:")
        print("cd result")
        print("python run_verification.py")

if __name__ == "__main__":
    main()

