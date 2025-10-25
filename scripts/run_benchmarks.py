#!/usr/bin/env python3

import subprocess
import resource
import os
import time
import argparse
import json

parser = argparse.ArgumentParser()
parser.add_argument('-s', '--save')
parser.add_argument('-c', '--compare')
parser.add_argument('-r', '--repeats', type=int, default=5)
parser.add_argument('-d', '--delay', type=float, default=0.2)

args = parser.parse_args()

EXE = './a.out'
BENCHMARKS_DIR = 'benchmarks/'

benchmarks = ['/dev/null'] + [BENCHMARKS_DIR + b for b in sorted(list(os.listdir(BENCHMARKS_DIR)))]

class Table():
    def __init__(self, *columns):
        self.columns = [str(c) for c in columns]
        self.widths = [len(c) for c in self.columns]
        self.just = [str.rjust] * len(columns)
        self.just[0] = str.ljust
        self.rows = []

    def add(self, *row):
        if len(row) != len(self.columns):
            raise RuntimeError('bad row length')
        row = [str(r) for r in row]
        self.widths = [max(w, len(r)) for w, r in zip(self.widths, row)]
        self.rows.append(row)

    def draw(self):
        def _draw_row(row):
            print(' | '.join((just(val,width) for just, val, width in zip(self.just, row, self.widths))))
        _draw_row(self.columns)
        for row in self.rows: _draw_row(row)


def measure_benchmark(benchmark, runs):
    times = []
    pages = []

    for _ in range(runs):
        time.sleep(args.delay)
        rbefore = resource.getrusage(resource.RUSAGE_CHILDREN)
        p = subprocess.run([EXE, '-s', benchmark], capture_output = True)
        rafter = resource.getrusage(resource.RUSAGE_CHILDREN)

        times.append((rafter.ru_utime - rbefore.ru_utime) * 1000)
        pages.append(rafter.ru_minflt - rbefore.ru_minflt)

    print(benchmark, times, pages)
    return { 'times': times, 'pages': pages }

average = lambda xs: sum(xs)/len(xs)
variance = lambda xs: sum((x**2 for x in xs))/len(xs) - average(xs)**2
stddev = lambda xs: variance(xs) ** 0.5

def test(xs, ys):
    xm = average(xs)
    ym = average(ys)
    effect = xm - ym
    var = (variance(xs)/len(xs) + variance(ys)/len(ys)) ** 0.5
    if var == 0:
        return effect, effect
    z = effect / (variance(xs)/len(xs) + variance(ys)/len(ys)) ** 0.5
    return z, effect

fmt_list = lambda xs: f'{average(xs):6.1f} +- {stddev(xs):6.1f}'

def fmt_stat(xs, ys):
    z, eff = test(xs, ys)
    eff_rel = eff / average(ys)
    return (
        fmt_list(xs), fmt_list(ys),
        f'{eff:.1f}', f'{eff_rel*100:+5.1f}%', f'{z:.2f}'
    )


results = { benchmark: measure_benchmark(benchmark, args.repeats) for benchmark in benchmarks }
    
if (args.compare):
    with open(args.compare, 'r') as f:
        other = json.load(f)

    t = Table(
            'Benchmark', 
            'Time / ms', 'Prev Time / ms', 'Effect', '%', 'Z', 
            'Page Faults', 'Prev Page Faults', 'Effect', '%', 'Z'
    )

    for name, stats in results.items():
        other_stats = other[name]
        t.add(
                name, 
                *fmt_stat(stats['times'], other_stats['times']),
                *fmt_stat(stats['pages'], other_stats['pages']),
        )
    t.draw()
else:
    t = Table('Benchmark', 'Time / ms', 'Page Faults')
    for name, stats in results.items():
        t.add(name, fmt_list(stats['times']), fmt_list(stats['pages']))
    t.draw()

if (args.save):
    with open(args.save, 'w') as f:
        json.dump(results, f)

