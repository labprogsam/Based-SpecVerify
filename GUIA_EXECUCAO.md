# Guia de Execução Seletiva - LLM-ReqVerif

Este guia explica como executar etapas isoladas do pipeline para reduzir o tempo de execução.

## 📦 Tarefas Disponíveis

O projeto contém **9 tarefas** disponíveis:

1. **`0_triplex`** - Triplex Monitor
2. **`1_fsm`** - Finite State Machine
3. **`2_tustin`** - Tustin Integrator
4. **`3_regulators`** ou **`3_regs`** - Regulators
5. **`4_nlguide`** - Non-Linear Guidance
6. **`5_nn`** - Neural Network
7. **`6_eb`** - Energy Balance
8. **`8_swim`** - SWIM (Speed Warning and Impact Monitor)
9. **`9_euler`** ou **`9_euler321_I2B`** - Euler 321 I2B Transformation

**Nota:** Não há tarefa 7 no projeto.

## 🎯 Visão Geral

O pipeline completo consiste em:
1. **Geração de Código LLM** (pode levar horas)
   - Etapa 1: Geração de especificação formal
   - Etapa 2: Geração de código de verificação
2. **Verificação ESBMC** (pode levar horas)

## 🚀 Opções de Execução

### Opção 1: Script Principal (`run_pipeline.py`)

O script principal permite executar o pipeline completo ou etapas isoladas:

```bash
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
```

### Opção 2: Scripts Individuais

#### Geração de Código (`result/generate_llm_code.py`)

```bash
cd result

# Executar ambas as etapas
python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude

# Executar apenas Etapa 1
python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase 1

# Executar apenas Etapa 2 (requer Etapa 1 já executada)
python generate_llm_code.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --only-phase 2
```

#### Verificação (`result/run_verification.py`)

```bash
cd result

# Executar todas as verificações
python run_verification.py

# Executar apenas para uma tarefa específica
python run_verification.py --task 1_fsm

# Executar apenas para um diretório LLM específico
python run_verification.py --llm-dir result/1_fsm/LLM_code

# Executar apenas uma propriedade específica
python run_verification.py --task 1_fsm --property 1

# Combinar filtros
python run_verification.py --task 1_fsm --property 1
```

## 📋 Exemplos Práticos

### Cenário 1: Executar apenas uma tarefa específica

```bash
# Gera código e verifica apenas para 1_fsm
python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude
```

### Cenário 2: Gerar código para múltiplas tarefas, verificar depois

```bash
# Gera código para tarefa 1
python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --skip-verification

# Gera código para tarefa 2
python run_pipeline.py --task 0_triplex --phase1 ChatGPT --phase2 Claude --skip-verification

# Verifica todas as tarefas depois
cd result
python run_verification.py
```

### Cenário 3: Verificar apenas uma propriedade específica

```bash
# Se o código já foi gerado, apenas verifica Property 1
python run_pipeline.py --task 1_fsm --skip-generation --property 1
```

### Cenário 4: Executar etapas em paralelo (manual)

```bash
# Terminal 1: Gera código para tarefa 1
python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude --skip-verification

# Terminal 2: Gera código para tarefa 2
python run_pipeline.py --task 0_triplex --phase1 ChatGPT --phase2 Claude --skip-verification

# Depois: Verifica todas
cd result
python run_verification.py
```

## ⚙️ Argumentos Disponíveis

### `run_pipeline.py`

**Geração:**
- `--task`: Nome da tarefa (obrigatório se não usar --skip-generation)
- `--phase1`: LLM para Etapa 1 (ChatGPT, Claude, Llama)
- `--phase2`: LLM para Etapa 2 (ChatGPT, Claude, Llama)
- `--config`: Arquivo de configuração JSON
- `--combination`: Nome da combinação no arquivo de configuração
- `--api-keys`: Arquivo JSON com API keys
- `--only-phase`: Executar apenas etapa 1 ou 2

**Verificação:**
- `--llm-dir`: Caminho completo para diretório LLM_code específico
- `--property`: Número da propriedade para verificar
- `--no-csv`: Não gerar arquivo CSV de resumo

**Controle:**
- `--skip-generation`: Pular etapa de geração
- `--skip-verification`: Pular etapa de verificação

### `generate_llm_code.py`

- `--task`: Nome da tarefa (obrigatório)
- `--phase1`: LLM para Etapa 1
- `--phase2`: LLM para Etapa 2
- `--config`: Arquivo de configuração JSON
- `--combination`: Nome da combinação
- `--api-keys`: Arquivo JSON com API keys
- `--only-phase`: Executar apenas etapa 1 ou 2

### `run_verification.py`

- `--task`: Filtrar por nome da tarefa
- `--llm-dir`: Caminho completo para diretório LLM_code
- `--property`: Número da propriedade para verificar
- `--no-csv`: Não gerar arquivo CSV de resumo

## 💡 Dicas

1. **Economia de tempo**: Use `--skip-verification` durante o desenvolvimento para testar apenas a geração de código
2. **Testes rápidos**: Use `--property` para verificar apenas uma propriedade específica
3. **Paralelização**: Execute gerações em paralelo em terminais diferentes
4. **Incremental**: Execute etapas isoladamente para depurar problemas específicos

## 📊 Tempo Estimado

- **Geração completa (Etapa 1 + 2)**: 1-3 horas por tarefa
- **Verificação completa**: 2-4 horas por tarefa
- **Pipeline completo**: 5-6 horas para todas as tarefas

Com execução seletiva, você pode reduzir significativamente o tempo necessário!

