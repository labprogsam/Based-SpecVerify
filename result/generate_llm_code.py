#!/usr/bin/env python3
"""
Script para gerar especificações formais e código de verificação
usando LLMs diferentes em cada etapa.

Uso:
    python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude
    python generate_llm_code.py --task 1_fsm --config llm_config.json --combination ChatGPT_Claude
"""

import os
import sys
import json
import argparse
from typing import Optional, Dict, Any
from dataclasses import dataclass
from pathlib import Path

@dataclass
class LLMConfig:
    """Configuração de uma LLM"""
    name: str
    api_key: str
    model: str
    provider: str  # "openai", "anthropic", "ollama"
    base_url: Optional[str] = None  # Para Ollama ou outros endpoints customizados

class LLMGenerator:
    """Classe para gerar código usando diferentes LLMs"""
    
    def __init__(self, phase1_llm: LLMConfig, phase2_llm: LLMConfig):
        """
        Args:
            phase1_llm: LLM para usar na Etapa 1 (Formalized Req & Test Design)
            phase2_llm: LLM para usar na Etapa 2 (Property Verification)
        """
        self.phase1_llm = phase1_llm
        self.phase2_llm = phase2_llm
    
    def call_llm(self, llm_config: LLMConfig, prompt: str, context: dict) -> str:
        """Chama a LLM especificada com o prompt e contexto"""
        try:
            if llm_config.provider == "openai":
                return self._call_openai(llm_config, prompt, context)
            elif llm_config.provider == "anthropic":
                return self._call_anthropic(llm_config, prompt, context)
            elif llm_config.provider == "ollama":
                return self._call_ollama(llm_config, prompt, context)
            else:
                raise ValueError(f"Provider não suportado: {llm_config.provider}")
        except Exception as e:
            print(f"Erro ao chamar LLM {llm_config.name}: {e}")
            raise
    
    def _call_openai(self, config: LLMConfig, prompt: str, context: dict) -> str:
        """Implementação para OpenAI (ChatGPT)"""
        try:
            from openai import OpenAI
        except ImportError:
            raise ImportError("Biblioteca 'openai' não instalada. Execute: pip install openai")
        
        client = OpenAI(api_key=config.api_key, base_url=config.base_url)
        response = client.chat.completions.create(
            model=config.model,
            messages=[{"role": "user", "content": prompt}],
            temperature=1  # Baixa temperatura para resultados mais consistentes
        )
        return response.choices[0].message.content
    
    def _call_anthropic(self, config: LLMConfig, prompt: str, context: dict) -> str:
        """Implementação para Anthropic (Claude)"""
        try:
            from anthropic import Anthropic
        except ImportError:
            raise ImportError("Biblioteca 'anthropic' não instalada. Execute: pip install anthropic")
        
        client = Anthropic(api_key=config.api_key)
        response = client.messages.create(
            model=config.model,
            max_tokens=4096,
            messages=[{"role": "user", "content": prompt}],
            temperature=1
        )
        return response.content[0].text
    
    def _call_ollama(self, config: LLMConfig, prompt: str, context: dict) -> str:
        """Implementação para Ollama (Llama local)"""
        try:
            import requests
        except ImportError:
            raise ImportError("Biblioteca 'requests' não instalada. Execute: pip install requests")
        
        base_url = config.base_url or "http://localhost:11434"
        response = requests.post(
            f"{base_url}/api/generate",
            json={
                "model": config.model,
                "prompt": prompt,
                "stream": False
            },
            timeout=600
        )
        response.raise_for_status()
        return response.json()["response"]
    
    def generate_formal_specification(self, c_code_path: str, reqs_path: str, 
                                     output_dir: str) -> str:
        """Etapa 1: Gera formal_specification.txt usando phase1_llm"""
        print(f"\n{'='*60}")
        print(f"Etapa 1: Gerando especificação formal usando {self.phase1_llm.name}")
        print(f"{'='*60}")
        
        # Lê o código C e requisitos
        if not os.path.exists(c_code_path):
            raise FileNotFoundError(f"Arquivo de código C não encontrado: {c_code_path}")
        if not os.path.exists(reqs_path):
            raise FileNotFoundError(f"Arquivo de requisitos não encontrado: {reqs_path}")
        
        with open(c_code_path, 'r', encoding='utf-8') as f:
            c_code = f.read()
        with open(reqs_path, 'r', encoding='utf-8') as f:
            requirements = f.read()
        
        # Lê o prompt1 apropriado
        script_dir = os.path.dirname(os.path.abspath(__file__))
        prompt1_path = os.path.join(script_dir, f"prompt1_{self.phase1_llm.name}.txt")
        
        if not os.path.exists(prompt1_path):
            # Tenta sem extensão ou com variações
            alternatives = [
                prompt1_path,
                os.path.join(script_dir, f"prompt1_{self.phase1_llm.name.lower()}.txt"),
                os.path.join(script_dir, "prompt1_Claude.txt"),  # Fallback
            ]
            for alt in alternatives:
                if os.path.exists(alt):
                    prompt1_path = alt
                    break
            else:
                raise FileNotFoundError(f"Prompt1 não encontrado para {self.phase1_llm.name}")
        
        with open(prompt1_path, 'r', encoding='utf-8') as f:
            prompt_template = f.read()
        
        # Monta o prompt completo
        full_prompt = f"""{prompt_template}

[Código C]
{c_code}

[Requisitos]
{requirements}
"""
        
        print(f"Chamando {self.phase1_llm.name} ({self.phase1_llm.model})...")
        # Chama a LLM da Etapa 1
        formal_spec = self.call_llm(self.phase1_llm, full_prompt, {
            'c_code': c_code,
            'requirements': requirements
        })
        
        # Salva o resultado
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, "formal_specification.txt")
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(formal_spec)
        
        print(f"✓ Especificação formal salva em: {output_path}")
        return output_path
    
    def generate_verification_code(self, formal_spec_path: str, c_code_path: str,
                                   output_dir: str, header_files: list = None) -> str:
        """Etapa 2: Gera ert_main.c usando phase2_llm"""
        print(f"\n{'='*60}")
        print(f"Etapa 2: Gerando código de verificação usando {self.phase2_llm.name}")
        print(f"{'='*60}")
        
        # Lê a especificação formal e código C
        if not os.path.exists(formal_spec_path):
            raise FileNotFoundError(f"Especificação formal não encontrada: {formal_spec_path}")
        if not os.path.exists(c_code_path):
            raise FileNotFoundError(f"Arquivo de código C não encontrado: {c_code_path}")
        
        with open(formal_spec_path, 'r', encoding='utf-8') as f:
            formal_spec = f.read()
        with open(c_code_path, 'r', encoding='utf-8') as f:
            c_code = f.read()
        
        # Lê headers se fornecidos
        headers_content = ""
        if header_files:
            for header_path in header_files:
                if os.path.exists(header_path):
                    with open(header_path, 'r', encoding='utf-8') as f:
                        headers_content += f"\n[Header: {os.path.basename(header_path)}]\n{f.read()}\n"
        
        # Lê o prompt2 apropriado
        script_dir = os.path.dirname(os.path.abspath(__file__))
        prompt2_path = os.path.join(script_dir, f"prompt2_{self.phase2_llm.name}.txt")
        
        if not os.path.exists(prompt2_path):
            # Tenta sem extensão ou com variações
            alternatives = [
                prompt2_path,
                os.path.join(script_dir, f"prompt2_{self.phase2_llm.name.lower()}.txt"),
                os.path.join(script_dir, "prompt2_Claude.txt"),  # Fallback
            ]
            for alt in alternatives:
                if os.path.exists(alt):
                    prompt2_path = alt
                    break
            else:
                raise FileNotFoundError(f"Prompt2 não encontrado para {self.phase2_llm.name}")
        
        with open(prompt2_path, 'r', encoding='utf-8') as f:
            prompt_template = f.read()
        
        # Monta o prompt completo
        full_prompt = f"""{prompt_template}

[formal specification.txt]
{formal_spec}

[functional c code]
{c_code}
{headers_content}
"""
        
        print(f"Chamando {self.phase2_llm.name} ({self.phase2_llm.model})...")
        # Chama a LLM da Etapa 2
        ert_main_code = self.call_llm(self.phase2_llm, full_prompt, {
            'formal_spec': formal_spec,
            'c_code': c_code
        })
        
        # Salva o resultado
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, "ert_main.c")
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(ert_main_code)
        
        print(f"✓ Código de verificação salvo em: {output_path}")
        return output_path

def load_config(config_path: str) -> Dict[str, Any]:
    """Carrega configuração de arquivo JSON"""
    with open(config_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def create_llm_config_from_dict(config_dict: Dict[str, Any], api_keys: Dict[str, str]) -> LLMConfig:
    """Cria LLMConfig a partir de dicionário de configuração"""
    api_key_env = config_dict.get('api_key_env', '')
    api_key = api_keys.get(api_key_env) or os.getenv(api_key_env)
    
    if not api_key:
        raise ValueError(f"API key não encontrada para {api_key_env}. "
                        f"Defina a variável de ambiente ou forneça no arquivo de configuração.")
    
    return LLMConfig(
        name=config_dict.get('name', ''),
        api_key=api_key,
        model=config_dict.get('model', ''),
        provider=config_dict.get('provider', ''),
        base_url=config_dict.get('base_url')
    )

def find_c_code_files(task_dir: str) -> tuple:
    """Encontra arquivos .c e .h no diretório da tarefa"""
    llm_code_dir = os.path.join(task_dir, "LLM_code")
    
    # Procura por qualquer subdiretório existente para pegar os arquivos .c e .h
    c_files = []
    h_files = []
    
    if os.path.exists(llm_code_dir):
        for item in os.listdir(llm_code_dir):
            item_path = os.path.join(llm_code_dir, item)
            if os.path.isdir(item_path):
                for file in os.listdir(item_path):
                    if file.endswith('.c') and file != 'ert_main.c':
                        c_files.append(os.path.join(item_path, file))
                    elif file.endswith('.h'):
                        h_files.append(os.path.join(item_path, file))
                if c_files:  # Se encontrou arquivos, usa esse diretório
                    break
    
    if not c_files:
        raise FileNotFoundError(f"Nenhum arquivo .c encontrado em {llm_code_dir}")
    
    return c_files[0], h_files[0] if h_files else None

def main():
    parser = argparse.ArgumentParser(
        description='Gera código de verificação usando LLMs diferentes em cada etapa',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos de uso:
  # Executar ambas as etapas
  python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude
  
  # Executar apenas Etapa 1 (especificação formal)
  python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase 1
  
  # Executar apenas Etapa 2 (código de verificação)
  python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase 2
  
  # Usar arquivo de configuração
  python generate_llm_code.py --task 1_fsm --config llm_config.json --combination ChatGPT_Claude
        """
    )
    parser.add_argument('--task', required=True, help='Nome da tarefa (ex: 1_fsm, 0_triplex)')
    parser.add_argument('--phase1', help='Nome da LLM para Etapa 1 (ex: ChatGPT, Claude)')
    parser.add_argument('--phase2', help='Nome da LLM para Etapa 2 (ex: ChatGPT, Claude)')
    parser.add_argument('--config', help='Arquivo de configuração JSON')
    parser.add_argument('--combination', help='Nome da combinação do arquivo de configuração')
    parser.add_argument('--api-keys', help='Arquivo JSON com API keys (opcional)')
    parser.add_argument('--only-phase', type=int, choices=[1, 2], 
                       help='Executar apenas uma etapa específica (1=Especificação Formal, 2=Código de Verificação)')
    
    args = parser.parse_args()
    
    # Determina diretório base
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    task_dir = os.path.join(script_dir, args.task)
    
    if not os.path.exists(task_dir):
        raise FileNotFoundError(f"Diretório da tarefa não encontrado: {task_dir}")
    
    # Carrega API keys se fornecido
    api_keys = {}
    if args.api_keys:
        if os.path.exists(args.api_keys):
            with open(args.api_keys, 'r') as f:
                api_keys = json.load(f)
        else:
            print(f"⚠️  Arquivo de API keys não encontrado: {args.api_keys}")
    else:
        # Tenta carregar automaticamente api_keys.json do diretório do script
        api_keys_path = os.path.join(script_dir, "api_keys.json")
        if os.path.exists(api_keys_path):
            with open(api_keys_path, 'r') as f:
                api_keys = json.load(f)
            print(f"✓ API keys carregadas de: {api_keys_path}")
        else:
            print(f"⚠️  Arquivo api_keys.json não encontrado em {script_dir}. "
                  f"Usando apenas variáveis de ambiente.")
    
    # Determina configurações das LLMs
    if args.config and args.combination:
        # Usa arquivo de configuração
        config = load_config(args.config)
        combination = None
        for comb in config.get('combinations', []):
            if comb['name'] == args.combination:
                combination = comb
                break
        
        if not combination:
            raise ValueError(f"Combinação '{args.combination}' não encontrada no arquivo de configuração")
        
        phase1_config = create_llm_config_from_dict(combination['phase1'], api_keys)
        phase2_config = create_llm_config_from_dict(combination['phase2'], api_keys)
        output_name = args.combination
    elif args.phase1 and args.phase2:
        # Usa argumentos da linha de comando
        # Configurações padrão
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
                'api_key_env': ''  # Ollama não precisa de API key
            }
        }
        
        # Configurações para Phase 2 - Claude mais recente disponível
        # Nota: claude-3-opus-20240229 está deprecated. Usando claude-3-5-sonnet-20240620
        default_configs_phase2 = {
            'ChatGPT': {
                'provider': 'openai',
                'model': 'gpt-5',
                'api_key_env': 'OPENAI_API_KEY'
            },
            'Claude': {
                'provider': 'anthropic',
                'model': 'claude-opus-4-1-20250805',  # Modelo mais recente disponível
                'api_key_env': 'ANTHROPIC_API_KEY'
            },
            'Llama': {
                'provider': 'ollama',
                'model': 'llama3.1',
                'api_key_env': ''  # Ollama não precisa de API key
            }
        }
        
        phase1_dict = default_configs_phase1.get(args.phase1, {})
        phase2_dict = default_configs_phase2.get(args.phase2, {})
        
        phase1_config = create_llm_config_from_dict({
            'name': args.phase1,
            **phase1_dict
        }, api_keys)
        phase2_config = create_llm_config_from_dict({
            'name': args.phase2,
            **phase2_dict
        }, api_keys)
        output_name = f"{args.phase1}_{args.phase2}"
    else:
        parser.error("Forneça --phase1 e --phase2 OU --config e --combination")
    
    # Cria o gerador
    generator = LLMGenerator(phase1_config, phase2_config)
    
    # Encontra arquivos necessários
    reqs_file = os.path.join(task_dir, f"{args.task}_reqs.txt")
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
    
    print(f"\n{'='*60}")
    print(f"Gerando código para tarefa: {args.task}")
    print(f"Etapa 1 (Especificação Formal): {phase1_config.name}")
    print(f"Etapa 2 (Código de Verificação): {phase2_config.name}")
    print(f"Diretório de saída: {output_dir}")
    if args.only_phase:
        print(f"⚠️  Modo filtrado: executando apenas Etapa {args.only_phase}")
    print(f"{'='*60}")
    
    import shutil
    
    # Etapa 1: Gera especificação formal
    if not args.only_phase or args.only_phase == 1:
        formal_spec_path = generator.generate_formal_specification(
            c_code_path=c_code_file,
            reqs_path=reqs_file,
            output_dir=output_dir
        )
    else:
        # Se executando apenas Etapa 2, tenta encontrar especificação existente
        formal_spec_path = os.path.join(output_dir, "formal_specification.txt")
        if not os.path.exists(formal_spec_path):
            raise FileNotFoundError(
                f"Especificação formal não encontrada em {formal_spec_path}. "
                f"Execute a Etapa 1 primeiro ou forneça o arquivo."
            )
        print(f"✓ Usando especificação formal existente: {formal_spec_path}")
    
    # Copia arquivos .c e .h necessários para o diretório de saída
    if not args.only_phase or args.only_phase == 1:
        if c_code_file:
            shutil.copy(c_code_file, output_dir)
            print(f"✓ Arquivo .c copiado para: {output_dir}")
        if h_file:
            shutil.copy(h_file, output_dir)
            print(f"✓ Arquivo .h copiado para: {output_dir}")
    
    # Etapa 2: Gera código de verificação
    if not args.only_phase or args.only_phase == 2:
        ert_main_path = generator.generate_verification_code(
            formal_spec_path=formal_spec_path,
            c_code_path=c_code_file,
            output_dir=output_dir,
            header_files=header_files
        )
    else:
        ert_main_path = os.path.join(output_dir, "ert_main.c")
        if not os.path.exists(ert_main_path):
            ert_main_path = None
    
    print(f"\n{'='*60}")
    print("✓ Geração concluída com sucesso!")
    print(f"{'='*60}")
    if not args.only_phase or args.only_phase == 1:
        print(f"Especificação formal: {formal_spec_path}")
    if not args.only_phase or args.only_phase == 2:
        print(f"Código de verificação: {ert_main_path}")
    
    if args.only_phase == 1:
        print(f"\nPara continuar, execute a Etapa 2:")
        print(f"  python generate_llm_code.py --task {args.task} --phase1 {phase1_config.name} --phase2 {phase2_config.name} --only-phase 2")
    elif args.only_phase == 2:
        print(f"\nPara executar a verificação, use:")
        print(f"  cd result")
        print(f"  python run_verification.py --task {args.task}")
    else:
        print(f"\nPara executar a verificação, use:")
        print(f"  cd result")
        print(f"  python run_verification.py --task {args.task}")

if __name__ == "__main__":
    main()

