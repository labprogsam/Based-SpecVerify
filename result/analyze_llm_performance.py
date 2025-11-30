#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script para analisar o desempenho das LLMs comparando com o Ground Truth
"""

import csv
from collections import defaultdict

def load_verification_summary(filename):
    """Carrega o arquivo verification_summary.csv"""
    data = {}
    with open(filename, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            task = row['Task']
            attempt = row['Attempt']
            if task not in data:
                data[task] = {}
            data[task][attempt] = row
    return data

def load_result_collection(filename):
    """Carrega o arquivo result collection.csv e extrai o Ground Truth"""
    ground_truth = {}
    with open(filename, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            task = row['Task']
            if row['Ground Truth'] == 'Ground Truth':
                ground_truth[task] = row
    return ground_truth

def normalize_value(value):
    """Normaliza valores para comparação"""
    if not value or value == 'N/A':
        return None
    return value.strip()

def is_verifiable(gt_value, result_value):
    """Verifica se uma propriedade pode ser comparada (não é N/A, Skipped, Error)"""
    if not gt_value or gt_value in ['N/A', 'Skipped', 'Error']:
        return False
    if not result_value or result_value in ['N/A', 'ERROR', 'Error']:
        return False
    return True

def is_correct(gt_value, result_value):
    """Verifica se o resultado está correto comparado com o Ground Truth"""
    # Mapeamento: SUCCESSFUL -> Valid, FAILED -> Falsifiable
    if gt_value == 'Valid' and result_value == 'SUCCESSFUL':
        return True
    if gt_value == 'Falsifiable' and result_value == 'FAILED':
        return True
    if gt_value == 'TIMEOUT' and result_value == 'TIMEOUT':
        return True
    return False

def is_verified(result_value):
    """Verifica se uma propriedade foi verificada (SUCCESSFUL ou FAILED)"""
    return result_value in ['SUCCESSFUL', 'FAILED']

def is_false_positive(gt_value, result_value):
    """False Positive: disse que é válido (SUCCESSFUL) mas na verdade é falsificável"""
    return gt_value == 'Falsifiable' and result_value == 'SUCCESSFUL'

def is_false_negative(gt_value, result_value):
    """False Negative: disse que é falsificável (FAILED) mas na verdade é válido"""
    return gt_value == 'Valid' and result_value == 'FAILED'

def analyze_performance(verification_data, ground_truth_data):
    """Analisa o desempenho das LLMs"""
    results = {}
    
    for task in verification_data:
        if task not in ground_truth_data:
            continue
            
        gt = ground_truth_data[task]
        
        for attempt in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
            if attempt not in verification_data[task]:
                continue
                
            result_row = verification_data[task][attempt]
            
            verified = 0  # Corretos
            formed = 0    # Verificados (SUCCESSFUL ou FAILED)
            total = 0     # Total de propriedades verificáveis
            false_positives = 0
            false_negatives = 0
            
            # Itera sobre todas as propriedades (até Property_14)
            for prop_num in range(1, 15):
                prop_name = f'Property_{prop_num}'
                gt_value = normalize_value(gt.get(prop_name))
                result_value = normalize_value(result_row.get(prop_name))
                
                if not is_verifiable(gt_value, result_value):
                    continue
                    
                total += 1
                
                if is_verified(result_value):
                    formed += 1
                    
                    if is_correct(gt_value, result_value):
                        verified += 1
                    elif is_false_positive(gt_value, result_value):
                        false_positives += 1
                    elif is_false_negative(gt_value, result_value):
                        false_negatives += 1
            
            if attempt not in results:
                results[attempt] = {
                    'verified': 0,
                    'formed': 0,
                    'total': 0,
                    'false_positives': 0,
                    'false_negatives': 0
                }
            
            results[attempt]['verified'] += verified
            results[attempt]['formed'] += formed
            results[attempt]['total'] += total
            results[attempt]['false_positives'] += false_positives
            results[attempt]['false_negatives'] += false_negatives
    
    return results

def generate_markdown_report(results):
    """Gera o relatório em markdown"""
    md = []
    md.append("# Análise de Desempenho das LLMs")
    md.append("")
    md.append("Este relatório compara os resultados das LLMs (`verification_summary.csv`) com o Ground Truth (`result collection.csv`).")
    md.append("")
    md.append("## Métricas Gerais")
    md.append("")
    md.append("| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives⋆ | FalseNegatives⋆ |")
    md.append("|--------|----------------------|---------------------|-----------------|-----------------|")
    
    for method in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
        if method not in results:
            continue
        r = results[method]
        verified = r['verified']
        formed = r['formed']
        total = r['total']
        fp = r['false_positives']
        fn = r['false_negatives']
        
        if total > 0:
            verification_rate = (verified / total) * 100
        else:
            verification_rate = 0.0
        
        md.append(f"| {method} | {verified}/{formed}/{total} | {verification_rate:.1f} | {fp} | {fn} |")
    
    md.append("")
    md.append("### Legenda")
    md.append("")
    md.append("- **Verified**: Número de propriedades verificadas corretamente")
    md.append("- **Formed**: Número de propriedades que foram verificadas (SUCCESSFUL ou FAILED)")
    md.append("- **Total**: Número total de propriedades verificáveis (excluindo N/A, Skipped, Error)")
    md.append("- **VerificationRate**: Percentual de propriedades verificadas corretamente em relação ao total")
    md.append("- **FalsePositives⋆**: Propriedades marcadas como SUCCESSFUL mas que são Falsifiable no Ground Truth")
    md.append("- **FalseNegatives⋆**: Propriedades marcadas como FAILED mas que são Valid no Ground Truth")
    md.append("")
    
    # Visualização gráfica
    md.append("## Visualização da Taxa de Verificação")
    md.append("")
    
    for method in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
        if method not in results:
            continue
        r = results[method]
        total = r['total']
        verified = r['verified']
        formed = r['formed']
        
        if total > 0:
            rate = (verified / total) * 100
            bar_length = 50
            filled = int(bar_length * verified / total)
            bar = '█' * filled + '░' * (bar_length - filled)
            md.append(f"```")
            md.append(f"{method:<20} {bar} {rate:.1f}% ({verified}/{formed}/{total})")
            md.append(f"```")
            md.append("")
    
    # Comparação lado a lado
    md.append("### Comparação Visual")
    md.append("")
    md.append("```")
    methods_list = ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']
    bar_length = 50
    
    for method in methods_list:
        if method in results:
            r = results[method]
            rate = (r['verified'] / r['total'] * 100) if r['total'] > 0 else 0
            filled = int(bar_length * r['verified'] / r['total']) if r['total'] > 0 else 0
            md.append(f"{method:<20} {'█' * filled}{'░' * (bar_length - filled)} {rate:.1f}%")
    md.append("```")
    md.append("")
    
    # Análise detalhada por tarefa
    md.append("## Análise Detalhada por Tarefa")
    md.append("")
    
    # Carregar dados novamente para análise detalhada
    verification_data = load_verification_summary('verification_summary.csv')
    ground_truth_data = load_result_collection('result collection.csv')
    
    for task in sorted(verification_data.keys()):
        if task not in ground_truth_data:
            continue
        
        md.append(f"### {task}")
        md.append("")
        md.append("| Método | Verified/Formed/Total | VerificationRate(%) | FalsePositives | FalseNegatives |")
        md.append("|--------|----------------------|---------------------|----------------|----------------|")
        
        gt = ground_truth_data[task]
        
        for attempt in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
            if attempt not in verification_data[task]:
                continue
            
            result_row = verification_data[task][attempt]
            
            verified = 0
            formed = 0
            total = 0
            false_positives = 0
            false_negatives = 0
            
            for prop_num in range(1, 15):
                prop_name = f'Property_{prop_num}'
                gt_value = normalize_value(gt.get(prop_name))
                result_value = normalize_value(result_row.get(prop_name))
                
                if not is_verifiable(gt_value, result_value):
                    continue
                    
                total += 1
                
                if is_verified(result_value):
                    formed += 1
                    
                    if is_correct(gt_value, result_value):
                        verified += 1
                    elif is_false_positive(gt_value, result_value):
                        false_positives += 1
                    elif is_false_negative(gt_value, result_value):
                        false_negatives += 1
            
            if total > 0:
                verification_rate = (verified / total) * 100
            else:
                verification_rate = 0.0
            
            md.append(f"| {attempt} | {verified}/{formed}/{total} | {verification_rate:.1f} | {false_positives} | {false_negatives} |")
        
        md.append("")
    
    # Resumo e conclusões
    md.append("## Resumo e Conclusões")
    md.append("")
    
    # Calcular totais únicos (não duplicados por método)
    methods_list = ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']
    available_methods = [m for m in methods_list if m in results]
    
    if available_methods:
        # O total de propriedades verificáveis é o mesmo para todos (mesmas tarefas)
        # Usar o primeiro método disponível como referência
        total_props = results[available_methods[0]]['total']
        total_fp = sum(r['false_positives'] for r in results.values())
        total_fn = sum(r['false_negatives'] for r in results.values())
        
        md.append(f"- **Total de propriedades verificáveis**: {total_props}")
        
        for method in methods_list:
            if method in results:
                r = results[method]
                rate = (r['verified'] / r['total'] * 100) if r['total'] > 0 else 0
                md.append(f"- **{method}**: {r['verified']}/{r['formed']}/{r['total']} - Taxa: {rate:.1f}%")
        
        md.append(f"- **Total de falsos positivos**: {total_fp}")
        md.append(f"- **Total de falsos negativos**: {total_fn}")
        md.append("")
    
    # Comparação entre métodos
    md.append("### Comparação entre Métodos")
    md.append("")
    
    methods_list = ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']
    method_rates = {}
    
    for method in methods_list:
        if method in results:
            r = results[method]
            rate = (r['verified'] / r['total'] * 100) if r['total'] > 0 else 0
            method_rates[method] = rate
            md.append(f"- **{method}**: {rate:.1f}% de taxa de verificação")
    
    if method_rates:
        best_method = max(method_rates.items(), key=lambda x: x[1])
        worst_method = min(method_rates.items(), key=lambda x: x[1])
        
        md.append(f"- **Melhor desempenho**: {best_method[0]} ({best_method[1]:.1f}%)")
        md.append(f"- **Pior desempenho**: {worst_method[0]} ({worst_method[1]:.1f}%)")
        md.append(f"- **Diferença**: {best_method[1] - worst_method[1]:.1f} pontos percentuais")
    
    # Análise de erros
    md.append("")
    md.append("## Análise de Erros")
    md.append("")
    md.append("### Distribuição de Erros")
    md.append("")
    md.append("| Método | False Positives | False Negatives | Total de Erros | Taxa de Erro (%) |")
    md.append("|--------|----------------|-----------------|----------------|------------------|")
    
    for method in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
        if method not in results:
            continue
        r = results[method]
        fp = r['false_positives']
        fn = r['false_negatives']
        total_errors = fp + fn
        formed = r['formed']
        error_rate = (total_errors / formed * 100) if formed > 0 else 0
        md.append(f"| {method} | {fp} | {fn} | {total_errors} | {error_rate:.1f} |")
    
    md.append("")
    md.append("### Interpretação dos Erros")
    md.append("")
    md.append("- **False Positives**: Propriedades marcadas como válidas (SUCCESSFUL) mas que são falsificáveis no Ground Truth")
    md.append("  - Indica que o método está sendo muito otimista, considerando propriedades válidas quando não são")
    md.append("- **False Negatives**: Propriedades marcadas como falsificáveis (FAILED) mas que são válidas no Ground Truth")
    md.append("  - Indica que o método está sendo muito conservador, rejeitando propriedades válidas")
    md.append("")
    
    # Tabela resumo por tarefa
    md.append("## Resumo por Tarefa")
    md.append("")
    md.append("| Tarefa | ChatGPT_Claude | ChatGPT_code | Claude_ChatGPT | Claude_code | Melhor Método |")
    md.append("|--------|---------------|-------------|----------------|------------|---------------|")
    
    verification_data = load_verification_summary('verification_summary.csv')
    ground_truth_data = load_result_collection('result collection.csv')
    
    for task in sorted(verification_data.keys()):
        if task not in ground_truth_data:
            continue
        
        gt = ground_truth_data[task]
        method_rates = {}
        
        for attempt in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
            if attempt not in verification_data[task]:
                continue
            
            result_row = verification_data[task][attempt]
            verified = 0
            total = 0
            
            for prop_num in range(1, 15):
                prop_name = f'Property_{prop_num}'
                gt_value = normalize_value(gt.get(prop_name))
                result_value = normalize_value(result_row.get(prop_name))
                
                if not is_verifiable(gt_value, result_value):
                    continue
                    
                total += 1
                
                if is_verified(result_value) and is_correct(gt_value, result_value):
                    verified += 1
            
            if total > 0:
                rate = (verified / total) * 100
                method_rates[attempt] = rate
        
        if method_rates:
            rates_str = []
            for method in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
                if method in method_rates:
                    rates_str.append(f"{method_rates[method]:.1f}%")
                else:
                    rates_str.append("N/A")
            
            # Determinar melhor método
            if method_rates:
                max_rate = max(method_rates.values())
                best_methods = [m for m, r in method_rates.items() if r == max_rate]
                
                if max_rate == 0:
                    best_method = "Empate (0%)"
                elif len(best_methods) == 1:
                    best_method = best_methods[0]
                else:
                    best_method = f"Empate ({', '.join(best_methods)})"
            else:
                best_method = "N/A"
            
            md.append(f"| {task} | {' | '.join(rates_str)} | {best_method} |")
    
    md.append("")
    
    return '\n'.join(md)

def main():
    verification_data = load_verification_summary('verification_summary.csv')
    ground_truth_data = load_result_collection('result collection.csv')
    
    results = analyze_performance(verification_data, ground_truth_data)
    
    markdown = generate_markdown_report(results)
    
    with open('llm_performance_analysis.md', 'w', encoding='utf-8') as f:
        f.write(markdown)
    
    print("Análise concluída! Arquivo 'llm_performance_analysis.md' gerado.")
    print("\nResumo:")
    for method in ['ChatGPT_Claude', 'ChatGPT_code', 'Claude_ChatGPT', 'Claude_code']:
        if method in results:
            r = results[method]
            rate = (r['verified'] / r['total'] * 100) if r['total'] > 0 else 0
            print(f"{method}: {r['verified']}/{r['formed']}/{r['total']} - {rate:.1f}%")

if __name__ == '__main__':
    main()

