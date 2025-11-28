# Guia de Uso: Geração de Código com LLMs Diferentes por Etapa

Este guia explica como usar o script `generate_llm_code.py` para gerar especificações formais e código de verificação usando LLMs diferentes em cada etapa do processo.

## 📋 Visão Geral

O processo de verificação tem duas etapas principais:

1. **Etapa 1 - Formalized Req & Test Design**: Gera `formal_specification.txt` analisando o código C e requisitos
2. **Etapa 2 - Property Verification**: Gera `ert_main.c` com asserções baseado na especificação formal

Agora você pode usar **LLMs diferentes** em cada etapa!

## 🚀 Instalação

1. Instale as dependências:
```bash
pip install -r requirements.txt
```

2. Configure as variáveis de ambiente com suas API keys:
```bash
# Para OpenAI (ChatGPT)
export OPENAI_API_KEY="sua-chave-aqui"

# Para Anthropic (Claude)
export ANTHROPIC_API_KEY="sua-chave-aqui"
```

Para Windows PowerShell:
```powershell
$env:OPENAI_API_KEY="sua-chave-aqui"
$env:ANTHROPIC_API_KEY="sua-chave-aqui"
```

## 📖 Uso Básico

### Método 1: Usando argumentos da linha de comando

```bash
python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude
```

Isso irá:
- Usar **ChatGPT** para gerar a especificação formal (Etapa 1)
- Usar **Claude** para gerar o código de verificação (Etapa 2)
- Salvar os resultados em `result/1_fsm/LLM_code/ChatGPT_Claude/`

### Método 2: Usando arquivo de configuração

1. Edite `llm_config.json` para adicionar suas combinações preferidas
2. Execute:
```bash
python generate_llm_code.py --task 1_fsm --config llm_config.json --combination ChatGPT_Claude
```

## 🔧 Configuração Avançada

### Arquivo de Configuração (llm_config.json)

O arquivo `llm_config.json` permite definir combinações pré-configuradas:

```json
{
  "combinations": [
    {
      "name": "ChatGPT_Claude",
      "phase1": {
        "name": "ChatGPT",
        "provider": "openai",
        "model": "gpt-5",
        "api_key_env": "OPENAI_API_KEY"
      },
      "phase2": {
        "name": "Claude",
        "provider": "anthropic",
        "model": "claude-3-5-sonnet-20241022",
        "api_key_env": "ANTHROPIC_API_KEY"
      }
    }
  ]
}
```
##### versoes de llms
claude: https://platform.claude.com/docs/en/about-claude/models/overview
chatgpt: GET - https://api.openai.com/v1/models (Autenticação de Bearer token sendo ele: Bearer api_key)

### Usando Ollama (Llama local)

Para usar modelos Llama executados localmente via Ollama:

1. Instale e inicie o Ollama:
```bash
# Instalar Ollama (veja https://ollama.ai)
ollama serve

# Em outro terminal, baixe um modelo:
ollama pull llama3.1
```

2. Use a combinação `Llama_Llama` ou crie uma nova:
```bash
python generate_llm_code.py --task 1_fsm --phase1 Llama --phase2 Llama
```

### API Keys em Arquivo (Opcional)

Você pode criar um arquivo `api_keys.json` (não commitado no git):

```json
{
  "OPENAI_API_KEY": "sua-chave-openai",
  "ANTHROPIC_API_KEY": "sua-chave-anthropic"
}
```

E usar:
```bash
python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --api-keys api_keys.json
```

## 📁 Estrutura de Diretórios

Após a geração, a estrutura será:

```
result/
  └── 1_fsm/
      └── LLM_code/
          ├── ChatGPT_Claude/          # Nova estrutura: combinação de LLMs
          │   ├── formal_specification.txt
          │   ├── ert_main.c
          │   ├── fsm_12B_global.c
          │   └── fsm_12B_global.h
          ├── Claude_ChatGPT/          # Outra combinação
          ├── ChatGPT_code/            # Estrutura antiga (compatível)
          └── Claude_code/             # Estrutura antiga (compatível)
```

## ✅ Verificação

Após gerar o código, execute a verificação:

```bash
cd result
python run_verification.py
```

O script `run_verification.py` foi atualizado para suportar tanto a estrutura antiga quanto a nova.

## 🎯 Exemplos de Uso

### Exemplo 1: ChatGPT → Claude
```bash
python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude
```

### Exemplo 2: Claude → ChatGPT
```bash
python generate_llm_code.py --task 1_fsm --phase1 Claude --phase2 ChatGPT
```

### Exemplo 3: Usando configuração
```bash
python generate_llm_code.py --task 0_triplex --config llm_config.json --combination Claude_ChatGPT
```

### Exemplo 4: Llama local
```bash
python generate_llm_code.py --task 1_fsm --phase1 Llama --phase2 Llama
```

## 🔍 Troubleshooting

### Erro: "API key não encontrada"
- Verifique se as variáveis de ambiente estão configuradas
- Ou forneça um arquivo `api_keys.json` com `--api-keys`

### Erro: "Prompt não encontrado"
- Certifique-se de que os arquivos `prompt1_*.txt` e `prompt2_*.txt` existem em `result/`
- O script tentará usar um fallback (prompt1_Claude.txt)

### Erro: "Biblioteca não instalada"
- Execute: `pip install -r requirements.txt`
- Para Ollama, certifique-se de que o servidor está rodando

### Erro: "Arquivo de código C não encontrado"
- Verifique se o diretório da tarefa existe em `result/`
- O script procura arquivos `.c` em subdiretórios de `LLM_code/`

## 📝 Notas

- Os prompts (`prompt1_*.txt` e `prompt2_*.txt`) são específicos para cada LLM
- O script copia automaticamente os arquivos `.c` e `.h` necessários para o diretório de saída
- A estrutura antiga (`ChatGPT_code`, `Claude_code`) continua funcionando normalmente
- O `run_verification.py` suporta ambas as estruturas automaticamente

## 🔄 Compatibilidade

O código gerado é compatível com o script de verificação existente (`run_verification.py`). Você pode:

1. Gerar código com diferentes combinações de LLMs
2. Executar verificação normalmente
3. Comparar resultados entre diferentes combinações

## 📊 Comparando Resultados

Para comparar resultados de diferentes combinações:

```bash
cd result
python run_verification.py
```

O arquivo `verification_summary.csv` conterá resultados de todas as combinações encontradas.

