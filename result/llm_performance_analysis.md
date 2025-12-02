# Análise de Desempenho das LLMs

Este relatório compara os resultados das LLMs (`verification_summary.csv`) com o Ground Truth (`result collection.csv`).

## Métricas Gerais

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives⋆ | FalseNegatives⋆ |
|--------|----------------------|---------------------|-----------------|-----------------|
| ChatGPT_Claude | 12/21/51 | 23.5 | 2 | 0 |
| ChatGPT_code | 13/17/54 | 24.1 | 0 | 1 |
| Claude_ChatGPT | 11/25/44 | 25.0 | 5 | 2 |
| Claude_code | 8/9/54 | 14.8 | 0 | 1 |

### Legenda

- **Verified**: Número de propriedades verificadas corretamente
- **Formed**: Número de propriedades que foram verificadas (SUCCESSFUL ou FAILED)
- **Total**: Número total de propriedades verificáveis (excluindo N/A, Skipped, Error)
- **VerificationRate**: Percentual de propriedades verificadas corretamente em relação ao total
- **FalsePositives⋆**: Propriedades marcadas como SUCCESSFUL mas que são Falsifiable no Ground Truth
- **FalseNegatives⋆**: Propriedades marcadas como FAILED mas que são Valid no Ground Truth

## Visualização da Taxa de Verificação

```
ChatGPT_Claude       ███████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 23.5% (12/21/51)
```

```
ChatGPT_code         ████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 24.1% (13/17/54)
```

```
Claude_ChatGPT       ████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 25.0% (11/25/44)
```

```
Claude_code          ███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 14.8% (8/9/54)
```

### Comparação Visual

```
ChatGPT_Claude       ███████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 23.5%
ChatGPT_code         ████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 24.1%
Claude_ChatGPT       ████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 25.0%
Claude_code          ███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 14.8%
```

## Análise Detalhada por Tarefa

### 0_triplex

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/0/4 | 0.0 | 0 | 0 |
| ChatGPT_code | 0/1/4 | 0.0 | 0 | 1 |
| Claude_ChatGPT | 1/2/4 | 25.0 | 0 | 1 |
| Claude_code | 0/1/4 | 0.0 | 0 | 1 |

### 1_fsm

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 12/13/13 | 92.3 | 1 | 0 |
| ChatGPT_code | 13/13/13 | 100.0 | 0 | 0 |
| Claude_ChatGPT | 8/13/13 | 61.5 | 4 | 1 |
| Claude_code | 6/6/13 | 46.2 | 0 | 0 |

### 2_tustin

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/0/3 | 0.0 | 0 | 0 |
| ChatGPT_code | 0/0/3 | 0.0 | 0 | 0 |
| Claude_ChatGPT | 2/3/3 | 66.7 | 1 | 0 |
| Claude_code | 2/2/3 | 66.7 | 0 | 0 |

### 3_regulators

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/1/8 | 0.0 | 0 | 0 |
| ChatGPT_code | 0/3/10 | 0.0 | 0 | 0 |
| Claude_ChatGPT | 0/0/0 | 0.0 | 0 | 0 |
| Claude_code | 0/0/10 | 0.0 | 0 | 0 |

### 4_nlguide

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/6/6 | 0.0 | 0 | 0 |
| ChatGPT_code | 0/0/7 | 0.0 | 0 | 0 |
| Claude_ChatGPT | 0/7/7 | 0.0 | 0 | 0 |
| Claude_code | 0/0/7 | 0.0 | 0 | 0 |

### 5_nn

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/0/4 | 0.0 | 0 | 0 |
| ChatGPT_code | 0/0/4 | 0.0 | 0 | 0 |
| Claude_ChatGPT | 0/0/4 | 0.0 | 0 | 0 |
| Claude_code | 0/0/4 | 0.0 | 0 | 0 |

### 6_eb

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/0/3 | 0.0 | 0 | 0 |
| ChatGPT_code | 0/0/3 | 0.0 | 0 | 0 |
| Claude_ChatGPT | 0/0/3 | 0.0 | 0 | 0 |
| Claude_code | 0/0/3 | 0.0 | 0 | 0 |

### 8_swim

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/1/2 | 0.0 | 1 | 0 |
| ChatGPT_code | 0/0/2 | 0.0 | 0 | 0 |
| Claude_ChatGPT | 0/0/2 | 0.0 | 0 | 0 |
| Claude_code | 0/0/2 | 0.0 | 0 | 0 |

### 9_euler

| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |
|--------|----------------------|---------------------|----------------|----------------|
| ChatGPT_Claude | 0/0/8 | 0.0 | 0 | 0 |
| ChatGPT_code | 0/0/8 | 0.0 | 0 | 0 |
| Claude_ChatGPT | 0/0/8 | 0.0 | 0 | 0 |
| Claude_code | 0/0/8 | 0.0 | 0 | 0 |

## Resumo e Conclusões

- **Total de propriedades verificáveis**: 51
- **ChatGPT_Claude**: 12/21/51 - Taxa: 23.5%
- **ChatGPT_code**: 13/17/54 - Taxa: 24.1%
- **Claude_ChatGPT**: 11/25/44 - Taxa: 25.0%
- **Claude_code**: 8/9/54 - Taxa: 14.8%
- **Total de falsos positivos**: 7
- **Total de falsos negativos**: 4

### Comparação entre Métodos

- **ChatGPT_Claude**: 23.5% de taxa de verificação
- **ChatGPT_code**: 24.1% de taxa de verificação
- **Claude_ChatGPT**: 25.0% de taxa de verificação
- **Claude_code**: 14.8% de taxa de verificação
- **Melhor desempenho**: Claude_ChatGPT (25.0%)
- **Pior desempenho**: Claude_code (14.8%)
- **Diferença**: 10.2 pontos percentuais

## Análise de Erros

### Distribuição de Erros

| Método | False Positives | False Negatives | Total de Erros | Taxa de Erro (%) |
|--------|----------------|-----------------|----------------|------------------|
| ChatGPT_Claude | 2 | 0 | 2 | 9.5 |
| ChatGPT_code | 0 | 1 | 1 | 5.9 |
| Claude_ChatGPT | 5 | 2 | 7 | 28.0 |
| Claude_code | 0 | 1 | 1 | 11.1 |

### Interpretação dos Erros

- **False Positives**: Propriedades marcadas como válidas (SUCCESSFUL) mas que são falsificáveis no Ground Truth
  - Indica que o método está sendo muito otimista, considerando propriedades válidas quando não são
- **False Negatives**: Propriedades marcadas como falsificáveis (FAILED) mas que são válidas no Ground Truth
  - Indica que o método está sendo muito conservador, rejeitando propriedades válidas

## Resumo por Tarefa

| Tarefa | ChatGPT_Claude | ChatGPT_code | Claude_ChatGPT | Claude_code | Melhor Método |
|--------|---------------|-------------|----------------|------------|---------------|
| 0_triplex | 0.0% | 0.0% | 25.0% | 0.0% | Claude_ChatGPT |
| 1_fsm | 92.3% | 100.0% | 61.5% | 46.2% | ChatGPT_code |
| 2_tustin | 0.0% | 0.0% | 66.7% | 66.7% | Empate (Claude_ChatGPT, Claude_code) |
| 3_regulators | 0.0% | 0.0% | N/A | 0.0% | Empate (0%) |
| 4_nlguide | 0.0% | 0.0% | 0.0% | 0.0% | Empate (0%) |
| 5_nn | 0.0% | 0.0% | 0.0% | 0.0% | Empate (0%) |
| 6_eb | 0.0% | 0.0% | 0.0% | 0.0% | Empate (0%) |
| 8_swim | 0.0% | 0.0% | 0.0% | 0.0% | Empate (0%) |
| 9_euler | 0.0% | 0.0% | 0.0% | 0.0% | Empate (0%) |
