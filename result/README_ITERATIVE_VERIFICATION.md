# Ciclo Iterativo de Verificação

Este documento explica como usar o ciclo iterativo de verificação que melhora automaticamente o código de verificação com base nos erros encontrados.

## 📋 Visão Geral

O script `run_iterative_verification.py` implementa um ciclo iterativo que:

1. **Gera código de verificação** inicial (Etapa 2)
2. **Executa verificação** usando ESBMC
3. **Extrai erros** encontrados (violated properties)
4. **Re-executa geração de código** incluindo os erros no prompt
5. **Repete** até não haver mais erros ou atingir limite de iterações

## 🚀 Uso Básico

### Exemplo 1: Verificar uma propriedade específica

```bash
python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --property 4
```

Isso irá:
- Gerar código de verificação para a Property 4
- Executar verificação
- Se houver erros, re-gerar código incluindo os erros
- Repetir até passar ou atingir 3 iterações (padrão)

### Exemplo 2: Verificar todas as propriedades

```bash
python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude
```

### Exemplo 3: Aumentar número de iterações

```bash
python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --property 4 --max-iterations 5
```

### Exemplo 4: Usar apenas Etapa 2 (especificação já existe)

```bash
python run_iterative_verification.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --property 4 --only-phase2
```

## 📝 Parâmetros

### Parâmetros Obrigatórios

- `--task`: Nome da tarefa (ex: `1_fsm`, `0_triplex`)
- `--phase1` e `--phase2`: OU `--config` e `--combination`

### Parâmetros Opcionais

- `--property`: Número da propriedade específica para verificar (padrão: todas)
- `--max-iterations`: Número máximo de iterações (padrão: 3)
- `--only-phase2`: Pula Etapa 1 e usa especificação existente
- `--api-keys`: Arquivo JSON com API keys
- `--config`: Arquivo de configuração JSON
- `--combination`: Nome da combinação no arquivo de configuração

## 🔄 Como Funciona

### Fluxo de Execução

```
Iteração 1:
  ├─ Gera ert_main.c (Etapa 2)
  ├─ Executa verificação
  ├─ Extrai erros (se houver)
  └─ Se houver erros → Iteração 2

Iteração 2:
  ├─ Gera ert_main.c incluindo erros anteriores
  ├─ Executa verificação
  ├─ Extrai erros (se houver)
  └─ Se houver erros → Iteração 3

Iteração 3:
  ├─ Gera ert_main.c incluindo erros anteriores
  ├─ Executa verificação
  └─ Se passar → Concluído ✓
```

### Extração de Erros

O script extrai automaticamente as seções "Violated property" dos arquivos de resultado da verificação. Cada erro contém:

- **Arquivo**: Caminho do arquivo onde o erro ocorreu
- **Linha/Coluna**: Localização do erro
- **Função**: Função onde o erro ocorreu
- **Mensagem**: Descrição do erro

Essas informações são incluídas no prompt da Etapa 2 para que a LLM possa corrigir o código.

## 📁 Estrutura de Arquivos

Após a execução, os arquivos são organizados assim:

```
result/
  └── 1_fsm/
      ├── LLM_code/
      │   └── ChatGPT_Claude/
      │       ├── formal_specification.txt
      │       ├── ert_main.c  (atualizado a cada iteração)
      │       └── ...
      └── verification_results/
          ├── ChatGPT_Claude_property_4_iter_1_result.txt
          ├── ChatGPT_Claude_property_4_iter_2_result.txt
          └── ...
```

## 💡 Exemplos Práticos

### Exemplo: Corrigir Property 4 com 5 iterações

```bash
cd result
python run_iterative_verification.py \
  --task 1_fsm \
  --phase1 ChatGPT \
  --phase2 Claude \
  --property 4 \
  --max-iterations 5
```

### Exemplo: Usar configuração de arquivo

```bash
python run_iterative_verification.py \
  --task 1_fsm \
  --config llm_config.json \
  --combination ChatGPT_Claude \
  --property 4
```

## ⚠️ Observações Importantes

1. **Custo de API**: Cada iteração faz chamadas à API da LLM. Use `--max-iterations` para controlar custos.

2. **Especificação Formal**: A Etapa 1 (especificação formal) é executada apenas uma vez na primeira iteração, a menos que você use `--only-phase2`.

3. **Propriedades**: Se não especificar `--property`, o script tentará verificar todas as propriedades encontradas no código.

4. **Erros**: O script para automaticamente quando todas as propriedades passam na verificação.

## 🔍 Saída do Script

O script exibe:

- Status de cada iteração
- Resultado de cada propriedade verificada
- Erros encontrados em cada iteração
- Resumo final com todas as iterações

Exemplo de saída:

```
============================================================
Iteração 1/3
============================================================

Gerando código de verificação (Etapa 2)...

  Verificando Property 4...
    ✗ Property 4 FALHOU - 1 erro(s) encontrado(s)
      Erro 1: Req4 violated: nominal->maneuver when sensor data not good...

  1 erro(s) encontrado(s). Preparando para próxima iteração...

============================================================
Iteração 2/3
============================================================
...
```

## 🐛 Troubleshooting

### Erro: "Nenhuma propriedade encontrada"
- Certifique-se de que o código `ert_main.c` contém definições `VERIFY_PROPERTY_X`
- Verifique se o diretório da tarefa está correto

### Erro: "API key não encontrada"
- Configure as variáveis de ambiente ou forneça `--api-keys`
- Veja `README_GENERATE_LLM.md` para mais detalhes

### Erro: "Especificação formal não encontrada"
- Execute sem `--only-phase2` na primeira vez
- Ou gere a especificação manualmente usando `generate_llm_code.py`

## 📚 Veja Também

- `README_GENERATE_LLM.md`: Como gerar código com LLMs
- `run_verification.py`: Script de verificação simples (sem ciclo iterativo)

