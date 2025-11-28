# Lista de Tarefas Disponíveis - LLM-ReqVerif

Este documento lista todas as tarefas disponíveis no projeto e seus detalhes.

## 📋 Tarefas Disponíveis

| ID | Nome | Descrição | Arquivos Principais |
|----|------|-----------|---------------------|
| `0_triplex` | Triplex Monitor | Sistema de monitoramento triplex | `triplex_12B.c`, `triplex_12B.h` |
| `1_fsm` | Finite State Machine | Máquina de estados finita | `fsm_12B_global.c`, `fsm_12B_global.h` |
| `2_tustin` | Tustin Integrator | Integrador de Tustin | `integrator_12B.c`, `integrator_12B.h` |
| `3_regulators` ou `3_regs` | Regulators | Sistema de reguladores | `regs_12B.c`, `regs_12B.h` |
| `4_nlguide` | Non-Linear Guidance | Sistema de guiamento não-linear | `NLGuidance_12B.c`, `NLGuidance_12B.h` |
| `5_nn` | Neural Network | Rede neural | `nn_12B.c`, `nn_12B.h` |
| `6_eb` | Energy Balance | Balanço de energia | `EB_12B.c`, `EB_12B.h` |
| `8_swim` | SWIM | Speed Warning and Impact Monitor | `swim_12B.c`, `swim_12B.h` |
| `9_euler` ou `9_euler321_I2B` | Euler 321 I2B | Transformação Euler 321 I2B | `euler321_I2B_12B.c`, `euler321_I2B_12B.h` |

## 📁 Estrutura de Diretórios

Cada tarefa segue a mesma estrutura:

```
result/
├── {task_id}_{task_name}/
│   ├── {task_id}_{task_name}_reqs.txt    # Requisitos
│   ├── LLM_code/                         # Código gerado por LLMs
│   │   ├── ChatGPT_code/
│   │   ├── Claude_code/
│   │   └── ...
│   ├── cocosim_output/                   # Resultados CoCoSim
│   ├── sldv_output/                       # Resultados SLDV
│   └── verification_results/             # Resultados da verificação ESBMC
```

## 🚀 Exemplos de Uso

### Executar uma tarefa específica

```bash
# Triplex Monitor
python run_pipeline.py --task 0_triplex --phase1 ChatGPT --phase2 Claude

# Finite State Machine
python run_pipeline.py --task 1_fsm --phase1 ChatGPT --phase2 Claude

# Tustin Integrator
python run_pipeline.py --task 2_tustin --phase1 ChatGPT --phase2 Claude

# Regulators
python run_pipeline.py --task 3_regulators --phase1 ChatGPT --phase2 Claude
# ou
python run_pipeline.py --task 3_regs --phase1 ChatGPT --phase2 Claude

# Non-Linear Guidance
python run_pipeline.py --task 4_nlguide --phase1 ChatGPT --phase2 Claude

# Neural Network
python run_pipeline.py --task 5_nn --phase1 ChatGPT --phase2 Claude

# Energy Balance
python run_pipeline.py --task 6_eb --phase1 ChatGPT --phase2 Claude

# SWIM
python run_pipeline.py --task 8_swim --phase1 ChatGPT --phase2 Claude

# Euler 321 I2B
python run_pipeline.py --task 9_euler --phase1 ChatGPT --phase2 Claude
# ou
python run_pipeline.py --task 9_euler321_I2B --phase1 ChatGPT --phase2 Claude
```

### Listar todas as tarefas disponíveis

```bash
# No diretório result/
ls -d */ | grep -E "^[0-9]_"
```

### Verificar quais tarefas têm código LLM gerado

```bash
cd result
python run_verification.py --help
# O script automaticamente encontra todas as tarefas com código LLM
```

## 📊 Estatísticas

- **Total de tarefas:** 9
- **Tarefas com código LLM:** Varia conforme execução
- **Tarefas com resultados de verificação:** Varia conforme execução

## 🔍 Verificação de Tarefas Disponíveis

Para verificar programaticamente quais tarefas estão disponíveis:

```python
import os

result_dir = "result"
tasks = [d for d in os.listdir(result_dir) 
         if os.path.isdir(os.path.join(result_dir, d)) 
         and d[0].isdigit()]

print("Tarefas disponíveis:")
for task in sorted(tasks):
    print(f"  - {task}")
```

