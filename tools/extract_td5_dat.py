"""Extract TD5.DAT files by their logical sizes and original relative paths"""
import argparse
import hashlib
import json
from pathlib import Path, PureWindowsPath
import struct


PROJECT = Path(__file__).resolve().parents[1]
SECTOR_BYTES = 2048
DIRECTORY_BYTES = 10 * SECTOR_BYTES
ENTRY_BYTES = 40


def digest(data):
    return hashlib.sha256(data).hexdigest()


def output_path(root, name):
    path = PureWindowsPath(name)
    if path.drive or path.root or not path.parts or any(part in ('.', '..') or ':' in part for part in path.parts):
        raise ValueError('Unsafe archive path: ' + name)
    if any(part.endswith((' ', '.')) for part in path.parts) or path.name.lower() == 'readme.md':
        raise ValueError('Refusing archive path: ' + name)
    target = root.joinpath(*path.parts)
    if not target.resolve().is_relative_to(root.resolve()):
        raise ValueError('Archive path escapes output directory: ' + name)
    return target


def write_verified(path, data):
    if path.exists():
        if not path.is_file() or path.read_bytes() != data:
            raise ValueError('Refusing to replace different existing output: ' + str(path))
        return 'reused'
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(data)
    return 'created'


def extract(archive, output, receipt):
    archive, output = Path(archive).resolve(), Path(output).resolve()
    rows, names, spans = [], set(), []
    archive_size = archive.stat().st_size
    with archive.open('rb') as stream:
        directory = stream.read(DIRECTORY_BYTES)
        if len(directory) != DIRECTORY_BYTES:
            raise ValueError('Truncated TD5 directory')
        inactive = False
        for index in range(DIRECTORY_BYTES // ENTRY_BYTES):
            record = directory[index * ENTRY_BYTES:(index + 1) * ENTRY_BYTES]
            size, sector = struct.unpack_from('<II', record, 32)
            if not size:
                inactive = True
                continue
            if inactive:
                raise ValueError('Non-contiguous active TD5 directory')
            if b'\0' not in record[:32]:
                raise ValueError('Unterminated TD5 filename')
            name = record[:32].split(b'\0', 1)[0].decode('ascii')
            target = output_path(output, name)
            key = target.relative_to(output).as_posix().casefold()
            if key in names or key in ('td5.dir', 'td5.pad', 'td5.dat'):
                raise ValueError('Duplicate or reserved TD5 output: ' + name)
            names.add(key)
            offset = (sector + 10) * SECTOR_BYTES
            padded = (size + SECTOR_BYTES - 1) & -SECTOR_BYTES
            if offset + padded > archive_size:
                raise ValueError('TD5 entry exceeds archive: ' + name)
            spans.append((offset, offset + padded))
            rows.append(dict(index=index, name=name, path=target.relative_to(output).as_posix(),
                             size=size, sector=sector, offset=offset, padded_size=padded))
        ordered = sorted(spans)
        if not rows or ordered[0][0] != DIRECTORY_BYTES or any(a[1] != b[0] for a, b in zip(ordered, ordered[1:])):
            raise ValueError('TD5 payload has overlapping entries or unrepresented gaps')
        if ordered[-1][1] != archive_size:
            raise ValueError('TD5 has an unrepresented trailing extent')
        for row in rows:
            stream.seek(row['offset'])
            data = stream.read(row['size'])
            if len(data) != row['size']:
                raise ValueError('Short TD5 entry read: ' + row['name'])
            row.update(sha256=digest(data))
            row['action'] = write_verified(output_path(output, row['name']), data)
        stream.seek(0)
        archive_hash = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            archive_hash.update(block)
    result = dict(schema=1, archive=str(archive), archive_sha256=archive_hash.hexdigest(),
                  archive_bytes=archive_size, output=str(output), entries=rows)
    receipt = Path(receipt)
    receipt.parent.mkdir(parents=True, exist_ok=True)
    receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, default=PROJECT / 'bin/DATA/TD5.DAT')
    parser.add_argument('--output', type=Path, default=PROJECT / 'bin/DATA')
    parser.add_argument('--receipt', type=Path, default=PROJECT / 'status/archive-extraction/manifest.json')
    args = parser.parse_args()
    result = extract(args.archive, args.output, args.receipt)
    print(json.dumps(dict(files=len(result['entries']), output=result['output'], receipt=str(args.receipt))))


if __name__ == '__main__':
    main()
