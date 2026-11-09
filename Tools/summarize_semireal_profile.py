"""Join real UE profiler frames to the private observer's population records.

Read-only inputs, create-only report. Reports Editor evidence, never silently
equates all-encounter/FPS/low-population samples with full-army RTS performance.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent
PREVIEW = ROOT / 'Artifacts/VQ01_Semireal1/GamePreview'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def number(value):
    result = float(value)
    require(math.isfinite(result), 'Non-finite metric')
    return result


def stats(values):
    ordered = sorted(values)
    require(bool(ordered), 'No measured values')
    quantile = lambda fraction: ordered[math.floor((len(ordered) - 1) * fraction)]
    return dict(count=len(ordered), mean=sum(ordered)/len(ordered), median=quantile(.5),
                p90=quantile(.9), p95=quantile(.95), p99=quantile(.99), maximum=ordered[-1])


def observed_settings(receipt):
    """Keep measured bounds distinct from source-based fixed-driver inference."""
    values = dict(line.split('=',1) for line in receipt.splitlines() if '=' in line)
    runtime = {key:value for key,value in values.items() if key.startswith('runtime.')}
    keys = ('primary_resolution_upper_min','primary_resolution_upper_max',
            'secondary_fraction_min','secondary_fraction_max','view_output_width','view_output_height')
    resolution = {key:number(values[key]) for key in keys if key in values}
    complete = len(resolution) == len(keys)
    full_fixed = complete and all(abs(resolution[key]-1.) <= 1.e-6 for key in keys[:4])
    full_fixed = full_fixed and resolution.get('view_output_width') == 1920 and resolution.get('view_output_height') == 1080
    full_fixed = full_fixed and runtime.get('runtime.r.DynamicRes.OperationMode') == '0'
    cap_keys = ('runtime.engine_battery_classification','runtime.r.DontLimitOnBattery','runtime.t.MaxFPS','runtime.smooth_frame_rate')
    battery_cap_predicate = None
    if all(key in runtime for key in cap_keys):
        battery_cap_predicate = [runtime[key] for key in cap_keys] == ['1','0','0','0']
    return dict(runtime=runtime,resolution_observations=resolution,
                fixed_full_scale_inferred=bool(full_fixed),
                editor_battery_cap_source_predicate_matches=battery_cap_predicate,
                inference_basis='Observed constant1.0 primary upper/secondary and disabled dynamic resolution; installed fixed legacy driver has upper bound equal to its fixed fraction. Not a protected getter or pixel-sized-buffer readback.')


def read_engine_csv(path):
    require(path.stat().st_size <= 512 * 1024 * 1024, 'CSV exceeds512MiB bounded parser')
    # Native FCsvStreamWriter appends new stat columns during the capture and
    # writes the FINAL union header at the end (HasHeaderRowAtEnd=1). Early data
    # rows legitimately have fewer columns. Never select only first-header width.
    headers = None
    with path.open(encoding='utf-8-sig', newline='') as handle:
        for row in csv.reader(handle):
            if row and row[0] == 'EVENTS' and 'FrameTime' in row:
                if headers is not None:
                    require(row[:len(headers)] == headers, 'Native CSV columns reordered, not append-only')
                headers = row
    require(headers is not None, 'No native CSV header')
    with path.open(encoding='utf-8-sig', newline='') as handle:
        reader = csv.reader(handle)
        measured = ('FrameTime', 'GameThreadTime', 'RenderThreadTime', 'GPUTime',
                    'RomanSemireal/EngineFrame', 'RomanSemireal/CandidateUnits',
                    'RomanSemireal/FPS', 'RomanEncounter/AliveUnits')
        for name in measured:
            require(headers.count(name) == 1, 'Missing/ambiguous measured column: ' + name)
        observer = [h for h in headers if h.endswith('/ObserverTick') and h.startswith('RomanSemireal/')]
        require(len(observer) == 1, 'Observer cost must be measured separately')
        selected = {name:headers.index(name) for name in (*measured, observer[0])}
        required_width = max(selected.values()) + 1
        frames = []
        for row in reader:
            if not row or (row[0] == 'EVENTS' and 'FrameTime' in row) or row[0] == '[HasHeaderRowAtEnd]':
                continue
            require(required_width <= len(row) <= len(headers), 'Malformed native data row width')
            frame_time = number(row[selected['FrameTime']])
            require(frame_time > 0, 'Non-positive native frame time')
            record = {name:number(row[index]) for name,index in selected.items()}
            for name in ('RomanSemireal/EngineFrame', 'RomanSemireal/CandidateUnits', 'RomanSemireal/FPS', 'RomanEncounter/AliveUnits'):
                require(record[name].is_integer(), 'Non-integer frame/population counter')
            frames.append(record)
        require(frames, 'No actual numeric CSV frames')
        require(any(row['GPUTime'] > 0 for row in frames), 'No measured GPU timing')
        return frames, observer[0]


def analyze(name):
    require(re.fullmatch(r'[A-Za-z0-9_]{1,48}', name), 'Safe capture name required')
    folder = PREVIEW / 'Artifacts/SemirealPerformance'
    native = folder / name
    process_path, exit_path = folder / (name+'.process.json'), folder / (name+'.exit.json')
    process = json.loads(process_path.read_text(encoding='utf-8-sig'))
    completed = json.loads(exit_path.read_text(encoding='utf-8-sig'))
    total = process['total']
    require(total in (50,100) and completed['exitCode'] == 0, 'Successful completed50/100 process required')
    automation_path = PREVIEW / 'Reports' / name / 'index.json'
    automation = json.loads(automation_path.read_text(encoding='utf-8-sig'))
    require(automation['failed'] == 0 and automation['inProcess'] == 0 and automation['notRun'] == 0,
            'Automation failed/pending/not-run')
    expected = f'RomanEmpireGame.Performance.Encounter{total}PIE'
    tests = [t for t in automation['tests'] if t['fullTestPath'] == expected]
    require(len(tests) == 1 and tests[0]['state'] in ('Success', 'SuccessWithWarnings'), 'Canonical encounter test must pass')
    population_path = native / 'population.csv'
    with population_path.open(encoding='utf-8-sig', newline='') as handle:
        population = {int(row['EngineFrame']):row for row in csv.DictReader(handle)}
    engine_path = folder / (name+'.csv')
    frames, observer_column = read_engine_csv(engine_path)
    native_messages = [e['event']['message'] for e in tests[0]['entries']]
    receipts = [re.search(r'; ([0-9]+) measured PIE frames;', message) for message in native_messages]
    receipt_counts = [int(match.group(1)) for match in receipts if match]
    require(len(receipt_counts) == 1 and abs(len(frames)-receipt_counts[0]) <= 8,
            'CSV count disagrees with canonical measured PIE-frame receipt')
    matched, skipped = [], 0
    for row in frames:
        frame = int(row['RomanSemireal/EngineFrame'])
        if frame not in population:
            skipped += 1
            continue
        sample = population[frame]
        # CSV custom counters can be updated within the same engine frame; the
        # observer's EngineFrame identity, not CSV row ordinal, is the join key.
        require(int(sample['Candidate']) == int(row['RomanSemireal/CandidateUnits']), 'Candidate counter join mismatch')
        require(int(sample['FPS']) == int(row['RomanSemireal/FPS']), 'Camera counter join mismatch')
        require(int(sample['Alive']) == int(sample['Candidate']), 'Not all registered actors use new art')
        require(number(sample['TimeDilation']) == 1., 'Measurement is not real time')
        matched.append((row,sample))
    require(skipped <= 8, 'Too many unmatched native frames; inspect capture timing')
    groups = {
        'all_joined_encounter':matched,
        'full_population_rts':[(r,p) for r,p in matched if int(p['Alive']) >= total and p['FPS'] == '0'],
        'full_population_fps':[(r,p) for r,p in matched if int(p['Alive']) >= total and p['FPS'] == '1'],
        'below_initial_population':[(r,p) for r,p in matched if int(p['Alive']) < total],
    }
    require(len(groups['full_population_rts']) >= 60, 'Insufficient full-population RTS samples')
    summaries = {}
    for label, pairs in groups.items():
        if not pairs:
            summaries[label] = dict(frames=0)
            continue
        metrics = {key:stats([r[key] for r,_ in pairs]) for key in ('FrameTime','GameThreadTime','RenderThreadTime','GPUTime',observer_column)}
        summaries[label] = dict(frames=len(pairs), metrics_ms=metrics,
            alive_counts=dict(Counter(p['Alive'] for _,p in pairs)),
            min_recently_on_screen=min(int(p['RecentlyOnScreen']) for _,p in pairs),
            min_projected_centers=min(int(p['ProjectedCenters']) for _,p in pairs),
            min_actor_ticks=min(int(p['ActorTicks']) for _,p in pairs),
            min_anim_ticks=min(int(p['AnimTicks']) for _,p in pairs),
            predicted_lod_counts={str(lod):stats([int(p[f'PredictedLOD{lod}']) for _,p in pairs]) for lod in range(3)},
            fps_from_mean_frame_time=1000./metrics['FrameTime']['mean'])
    primary = summaries['full_population_rts']['metrics_ms']['FrameTime']
    settings = observed_settings((native/'profile-receipt.txt').read_text(encoding='utf-8-sig'))
    paths = [engine_path,population_path,automation_path,process_path,exit_path,
             native/'profile-receipt.txt',native/'PreCapture.png']
    return dict(schema=3,name=name,initial_total=total,capture_valid=True,
        scope='Provisional Editor D3D12/SM6,1080p output; see explicit resolution observations/inference, not a packaged-player gate',
        observed_settings=settings,
        parser_uses_final_union_header=True,canonical_measured_pie_frames=receipt_counts[0],
        csv_numeric_frames=len(frames),joined_frames=len(matched),unmatched_frames=skipped,
        quantile='floor((n-1)*p), same convention as existing profiler summarizer',
        groups=summaries,frame_budget_ms=1000./60,
        primary_mean_within_60fps_budget=primary['mean'] <= 1000./60,
        primary_p95_within_60fps_budget=primary['p95'] <= 1000./60,
        performance_target_accepted=False,owner_approved=False,
        power_scheme=process['power_scheme'],battery=process['battery'],
        process_only_diagnostics=process.get('process_only_diagnostics',{}),
        files=[dict(path=str(p.relative_to(ROOT)),sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in paths])


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--name',required=True)
    parser.add_argument('--report',required=True)
    args = parser.parse_args()
    require(re.fullmatch(r'[A-Za-z0-9_-]+\.json',args.report), 'Simple fresh JSON report filename required')
    destination = PREVIEW / 'Artifacts/SemirealPerformance' / args.report
    require(not destination.exists(), 'Report exists; never overwrite')
    result = analyze(args.name)
    with destination.open('x',encoding='utf-8') as handle:
        json.dump(result,handle,indent=2)
    print(json.dumps({k:result[k] for k in ('name','initial_total','csv_numeric_frames','joined_frames','primary_mean_within_60fps_budget','primary_p95_within_60fps_budget')},indent=2))
