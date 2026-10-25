"""Losslessly split TD5 GAME.XA and PRES.XA into their original XA channels"""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile

PROJECT = Path(__file__).resolve().parents[1]
SECTOR_BYTES = 2352
SYNC = b'\x00' + b'\xff' * 10 + b'\x00'


def digest(path):
    result = hashlib.sha256()
    with Path(path).open('rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def split_archive(archive, output):
    rows = [dict(channel=n, sectors=0, ended=False, first_sector=None, last_sector=None) for n in range(8)]
    handles = []
    try:
        for row in rows:
            row['output'] = f'{archive.stem}_{row["channel"]}.XA'
            handles.append((output / row['output']).open('xb'))
        with archive.open('rb') as source:
            ordinal = 0
            while True:
                raw = source.read(SECTOR_BYTES)
                if not raw:
                    break
                if len(raw) != SECTOR_BYTES or raw[:12] != SYNC or raw[15] != 2 or raw[16:20] != raw[20:24]:
                    raise ValueError(f'Invalid XA sector {ordinal} in {archive}')
                if raw[18] & 4:
                    channel = raw[17]
                    if raw[16] != 1 or channel >= 8 or raw[19] != 1:
                        raise ValueError(f'Unexpected XA stream at sector {ordinal}')
                    row = rows[channel]
                    if row['ended'] or ordinal != channel + row['sectors'] * 8:
                        raise ValueError(f'Broken XA interleave at sector {ordinal}')
                    handles[channel].write(raw)
                    if row['first_sector'] is None:
                        row['first_sector'] = ordinal
                    row['last_sector'] = ordinal
                    row['sectors'] += 1
                    row['ended'] = bool(raw[18] & 128)
                ordinal += 1
        if any(not r['ended'] or not r['sectors'] for r in rows):
            raise ValueError('Missing channel or XA EOF marker: ' + str(archive))
    finally:
        for handle in handles:
            handle.close()
    for row in rows:
        path = output / row['output']
        row.update(bytes=path.stat().st_size, sha256=digest(path),
                   frames_44100=row['sectors'] * 2352,
                   seconds=row['sectors'] * 2352 / 44100)
    return rows


def extract(source, output, receipt):
    source, output = Path(source).resolve(), Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    tracks = []
    with tempfile.TemporaryDirectory(prefix='td5-xa-', dir=output) as temporary:
        temporary = Path(temporary)
        for stem in ['GAME', 'PRES']:
            archive = source / (stem + '.XA')
            rows = split_archive(archive, temporary)
            source_hash = digest(archive)
            for row in rows:
                row.update(source=archive.name, source_sha256=source_hash,
                           game_track=row['channel'] + (8 if stem == 'GAME' else 0))
                tracks.append(row)
        for row in tracks:
            (temporary / row['output']).replace(output / row['output'])
    result = dict(schema=1, codec='Original XA ADPCM, no transcoding', sector_bytes=2352,
                  sample_rate=37800, channels=2, output=str(output), tracks=tracks)
    receipt = Path(receipt)
    receipt.parent.mkdir(parents=True, exist_ok=True)
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=PROJECT / 'bin/DATA')
    parser.add_argument('--output', type=Path, default=PROJECT / 'bin/MUSIC')
    parser.add_argument('--receipt', type=Path, default=PROJECT / 'status/xa-music/extraction.json')
    args = parser.parse_args()
    result = extract(args.input, args.output, args.receipt)
    print(json.dumps(dict(tracks=len(result['tracks']), receipt=str(args.receipt))))


if __name__ == '__main__':
    main()
