#!/usr/bin/env python3
"""
Rabbit Simulation Analysis and Visualization Tool
Generates comprehensive graphs for analyzing population dynamics and behavior
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from glob import glob
import warnings
warnings.filterwarnings('ignore')

# Set style for better-looking plots
sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (14, 8)
plt.rcParams['font.size'] = 10


class RabbitSimulationAnalyzer:
    """Analyzes rabbit simulation data and generates visualizations"""
    
    def __init__(self):
        self.individual_data = []  # List of DataFrames from individual simulations
        self.summary_data = None    # Summary statistics across all simulations
        self.load_data()
    
    def load_data(self):
        """Load all simulation CSV files"""
        # Load individual simulation data
        sim_files = sorted(glob("simulation_*_pop*.csv"))
        sim_files = [f for f in sim_files if "summary" not in f]
        
        for file in sim_files:
            try:
                df = pd.read_csv(file)
                df['simulation_file'] = file
                self.individual_data.append(df)
                print(f"Loaded: {file}")
            except Exception as e:
                print(f"Error loading {file}: {e}")
        
        # Load summary data
        summary_files = glob("simulation_summary_*.csv")
        if summary_files:
            try:
                # Skip comment lines and read from CSV
                self.summary_data = pd.read_csv(summary_files[0], skiprows=6)
                print(f"Loaded summary: {summary_files[0]}")
            except Exception as e:
                print(f"Error loading summary: {e}")
    
    def plot_population_over_time(self):
        """Plot 1: Population dynamics over time for each simulation"""
        if not self.individual_data:
            print("No individual data to plot")
            return
        
        fig, ax = plt.subplots(figsize=(14, 7))

        # Distinct plotting styles per simulation
        markers = ['o', 's', '^', 'D', 'v', 'P', 'X', '*', '<', '>']
        linestyles = ['-', '--', '-.', ':']
        colors = sns.color_palette('tab10', n_colors=max(3, len(self.individual_data)))

        for idx, df in enumerate(self.individual_data):
            m = markers[idx % len(markers)]
            ls = linestyles[idx % len(linestyles)]
            c = colors[idx % len(colors)]
            ax.plot(df['Month'], df['Total_Alive'], marker=m, linestyle=ls,
                   color=c, label=df['simulation_file'].iloc[0], linewidth=1.8, markersize=5, alpha=0.9)
        
        ax.set_xlabel('Month', fontsize=12, fontweight='bold')
        ax.set_ylabel('Population (Total Alive Rabbits)', fontsize=12, fontweight='bold')
        ax.set_title('Population Over Time', fontsize=14, fontweight='bold')
        ax.legend(loc='best', fontsize=9)
        ax.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig('01_population_over_time.png', dpi=300, bbox_inches='tight')
        print("✓ Saved: 01_population_over_time.png")
        plt.close()
    
    def plot_growth_rate_over_time(self):
        """Plot 2: Growth rate (month-to-month change) over time"""
        if not self.individual_data:
            return
        
        fig, ax = plt.subplots(figsize=(14, 7))
        
        for df in self.individual_data:
            # Calculate growth rate: (current - previous) / previous * 100
            growth_rate = df['Total_Alive'].pct_change() * 100
            ax.plot(df['Month'][1:], growth_rate[1:], marker='s', 
                   label=df['simulation_file'].iloc[0], linewidth=2, markersize=3, alpha=0.7)
        
        ax.axhline(y=0, color='black', linestyle='--', linewidth=1, alpha=0.5)
        ax.set_xlabel('Month', fontsize=12, fontweight='bold')
        ax.set_ylabel('Growth Rate (%)', fontsize=12, fontweight='bold')
        ax.set_title('Monthly Population Growth Rate', fontsize=14, fontweight='bold')
        ax.legend(loc='best', fontsize=9)
        ax.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig('02_growth_rate_over_time.png', dpi=300, bbox_inches='tight')
        print("✓ Saved: 02_growth_rate_over_time.png")
        plt.close()
    
    def plot_phase_plot(self):
        """Plot 3: Phase plot (current population vs next month population)"""
        if not self.individual_data:
            return
        
        fig, axes = plt.subplots(1, len(self.individual_data), figsize=(6*len(self.individual_data), 5))
        if len(self.individual_data) == 1:
            axes = [axes]
        
        for idx, df in enumerate(self.individual_data):
            pop = df['Total_Alive'].values
            ax = axes[idx]
            
            # Plot current vs next month population
            ax.scatter(pop[:-1], pop[1:], alpha=0.6, s=50, c=df['Month'][:-1], cmap='viridis')
            
            # Add diagonal line (no change)
            min_pop, max_pop = pop.min(), pop.max()
            ax.plot([min_pop, max_pop], [min_pop, max_pop], 'r--', linewidth=2, label='No change')
            
            ax.set_xlabel('Population at Month t', fontsize=11, fontweight='bold')
            ax.set_ylabel('Population at Month t+1', fontsize=11, fontweight='bold')
            ax.set_title(f'Phase Plot - {df["simulation_file"].iloc[0]}', fontsize=12, fontweight='bold')
            ax.legend()
            ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig('03_phase_plot.png', dpi=300, bbox_inches='tight')
        print("✓ Saved: 03_phase_plot.png")
        plt.close()
    
    def plot_population_structure(self):
        """Plot 4: Sex distribution over time and final distribution"""
        if not self.individual_data:
            return
        
        # Create subplots for each simulation
        n_sims = len(self.individual_data)
        fig, axes = plt.subplots(n_sims, 2, figsize=(14, 4*n_sims))
        
        # Handle case with single simulation
        if n_sims == 1:
            axes = [axes]
        
        for idx, df in enumerate(self.individual_data):
            # Plot 1: Males vs Females over time for this simulation
            ax = axes[idx] if n_sims == 1 else axes[idx, 0]
            ax.plot(df['Month'], df['Males'], label='Males', marker='o', linewidth=2, markersize=4, color='blue')
            ax.plot(df['Month'], df['Females'], label='Females', marker='s', linewidth=2, markersize=4, color='red')
            ax.set_xlabel('Month', fontweight='bold')
            ax.set_ylabel('Count', fontweight='bold')
            ax.set_title(f'Sex Distribution Over Time - {df["simulation_file"].iloc[0]}', fontweight='bold')
            ax.legend()
            ax.grid(True, alpha=0.3)
            
            # Plot 2: Final sex ratio pie chart
            ax = axes[idx] if n_sims == 1 else axes[idx, 1]
            latest_month = df[df['Month'] == df['Month'].max()]
            if not latest_month.empty:
                males = latest_month['Males'].values[0]
                females = latest_month['Females'].values[0]
                colors = ['blue', 'red']
                ax.pie([males, females], labels=['Males', 'Females'], autopct='%1.1f%%', 
                       startangle=90, colors=colors)
                ax.set_title(f'Final Sex Ratio (Month {df["Month"].max()})', fontweight='bold')
        
        plt.tight_layout()
        plt.savefig('04_population_structure.png', dpi=300, bbox_inches='tight')
        print("✓ Saved: 04_population_structure.png")
        plt.close()
    
    def plot_births_vs_deaths(self):
        """Plot 5: Births vs Deaths over time"""
        if not self.individual_data:
            return
        
        fig, axes = plt.subplots(1, 2, figsize=(14, 6))
        
        all_data = pd.concat(self.individual_data, ignore_index=True)
        
        # Plot 1: Births and deaths
        ax = axes[0]
        ax.bar(all_data['Month'] - 0.2, all_data['Births'], width=0.4, label='Births', alpha=0.8)
        ax.bar(all_data['Month'] + 0.2, all_data['Deaths'], width=0.4, label='Deaths', alpha=0.8)
        ax.set_xlabel('Month', fontweight='bold')
        ax.set_ylabel('Count', fontweight='bold')
        ax.set_title('Monthly Births vs Deaths', fontweight='bold')
        ax.legend()
        ax.grid(True, alpha=0.3, axis='y')
        
        # Plot 2: Net change (births - deaths)
        ax = axes[1]
        net_change = all_data['Births'] - all_data['Deaths']
        colors = ['green' if x > 0 else 'red' for x in net_change]
        ax.bar(all_data['Month'], net_change, color=colors, alpha=0.7)
        ax.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
        ax.set_xlabel('Month', fontweight='bold')
        ax.set_ylabel('Net Change (Births - Deaths)', fontweight='bold')
        ax.set_title('Net Population Change Per Month', fontweight='bold')
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        plt.savefig('05_births_vs_deaths.png', dpi=300, bbox_inches='tight')
        print("✓ Saved: 05_births_vs_deaths.png")
        plt.close()
    

    

    
    def plot_extinction_outcomes(self):
        """Plot 8: Final population and survival vs extinction"""
        if self.summary_data is None or self.summary_data.empty:
            print("No summary data available for extinction analysis")
            return
        
        fig, axes = plt.subplots(1, 2, figsize=(14, 6))
        
        data = self.summary_data
        
        # Plot 1: Final population distribution
        ax = axes[0]
        ax.bar(data['Sim_Number'], data['Final_Alive'], color='steelblue', alpha=0.7)
        ax.set_xlabel('Simulation #', fontweight='bold')
        ax.set_ylabel('Final Population', fontweight='bold')
        ax.set_title('Final Population by Simulation', fontweight='bold')
        ax.grid(True, alpha=0.3, axis='y')
        
        # Plot 2: Extinction months / Survival vs Extinction
        ax = axes[1]
        extinct = data[data['Extinction_Month'] > 0]
        surviving = data[data['Extinction_Month'] == 0]
        
        if not extinct.empty:
            ax.bar(extinct['Sim_Number'], extinct['Extinction_Month'], 
                   label=f'Extinct ({len(extinct)})', color='red', alpha=0.7)
        if not surviving.empty:
            ax.bar(surviving['Sim_Number'], surviving['Months_Simulated'], 
                   label=f'Surviving ({len(surviving)})', color='green', alpha=0.7)
        
        ax.set_xlabel('Simulation #', fontweight='bold')
        ax.set_ylabel('Month', fontweight='bold')
        ax.set_title('Survival vs Extinction', fontweight='bold')
        ax.legend()
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        plt.savefig('08_extinction_outcomes.png', dpi=300, bbox_inches='tight')
        print("✓ Saved: 08_extinction_outcomes.png")
        plt.close()
    

    

    
    def generate_all_plots(self):
        """Generate all visualization plots"""
        print("\n" + "="*60)
        print("RABBIT SIMULATION ANALYSIS - GENERATING PLOTS")
        print("="*60 + "\n")
        
        if not self.individual_data:
            print("ERROR: No simulation data found. Please run the C simulation first.")
            return
        
        self.plot_population_over_time()
        self.plot_growth_rate_over_time()
        self.plot_phase_plot()
        self.plot_population_structure()
        self.plot_births_vs_deaths()
        self.plot_extinction_outcomes()
        
        print("\n" + "="*60)
        print("✓ ALL PLOTS GENERATED SUCCESSFULLY")
        print("="*60)
        print("\nOutput files:")
        print("  01_population_over_time.png - Main population trajectory")
        print("  02_growth_rate_over_time.png - Monthly population changes")
        print("  03_phase_plot.png - Population dynamics (current vs next month)")
        print("  04_population_structure.png - Sex distribution over time and final distribution")
        print("  05_births_vs_deaths.png - Reproduction and mortality")
        print("  08_extinction_outcomes.png - Final population and survival vs extinction")


def main():
    """Main entry point"""
    analyzer = RabbitSimulationAnalyzer()
    analyzer.generate_all_plots()


if __name__ == "__main__":
    main()
