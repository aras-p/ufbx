import gzip
import os
import re

def fetch_dumps(file):
    files = { }
    cur_file = None
    cur_dump = None
    for line in file:
        line = line.rstrip()
        if line.startswith("=="):
            name = line[2:].strip()
            cur_file = { }
            files[name] = cur_file
        elif line.startswith("--"):
            name = line[2:].strip()
            cur_dump = []
            cur_file[name] = cur_dump
        elif cur_dump is not None:
            cur_dump.append(line)
    return files

def diff_dump(dump, ref, file, runner, ref_runner):
    num_lines = max(len(dump), len(ref))
    stack = []

    for ix in range(num_lines):
        dline = dump[ix] if ix < len(dump) else ""
        rline = ref[ix] if ix < len(ref) else ""

        if dline == rline:
            if "{" in dline:
                stack.append(dline)
            elif "}" in dline:
                stack.pop()
        else:
            span = 2
            start = max(0, ix - span)
            stop = ix + span + 1

            print(f"== {file}")
            print()
            print(" ".join(s.strip().rstrip("{ ") for s in stack))

            print()
            print(f"-- {runner}")
            print()
            for lix in range(start, stop):
                dl = dump[lix] if lix < len(dump) else ""
                rl = ref[lix] if lix < len(ref) else ""
                prefix = "> " if dl != rl else "  "
                print(prefix + dl)

            print()
            print(f"-- {ref_runner}")
            print()
            for lix in range(start, stop):
                dl = dump[lix] if lix < len(dump) else ""
                rl = ref[lix] if lix < len(ref) else ""
                prefix = "> " if dl != rl else "  "
                print(prefix + rl)

            print()
            break

def do_compress(argv):
    with gzip.open(argv.o, "wt", compresslevel=8) as outf:
        for file in os.listdir(argv.directory):
            name = file[:-4] if file.endswith(".txt") else file
            path = os.path.join(argv.directory, file)
            print(f"== {name}", file=outf)
            with open(path, "rt") as inf:
                outf.writelines(inf)

def do_list(argv):
    entries = set()
    for file in os.listdir(argv.directory):
        path = os.path.join(argv.directory, file)
        with gzip.open(path, "rt") as inf:
            for line in inf:
                line = line.strip()
                if not line:
                    continue
                m = re.match(r"--\s*(\d+)\s+(.+)", line)
                if m:
                    frame = m.group(1)
                    path = m.group(2)
                    entries.add((frame, path))
    with open(argv.o, "wt") as outf:
        for frame, path in entries:
            print(f"0000000000000000 {frame} {path}", file=outf)

def do_diff(argv):
    with gzip.open(argv.ref, "rt") as inf:
        ref_dumps = fetch_dumps(inf)
    ref_runner, ref_file = next(iter(ref_dumps.items()))

    for file in os.listdir(argv.directory):
        path = os.path.join(argv.directory, file)
        with gzip.open(path, "rt") as inf:
            dumps = fetch_dumps(inf)
            for runner, files in dumps.items():
                for file, dump in files.items():
                    ref = ref_file[file]
                    diff_dump(dump, ref, file, runner, ref_runner)

if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser(prog="hash_diff.py")
    parser.add_argument("--verbose", action="store_true", help="Show extra information")

    subparsers = parser.add_subparsers(metavar="cmd")

    parser_compress = subparsers.add_parser("compress", help="Compress files")
    parser_compress.add_argument("directory", help="Directory of hash files to compress")
    parser_compress.add_argument("-o", metavar="output.gz", required=True, help="Output .gz filename")
    parser_compress.set_defaults(func=do_compress)

    parser_dump = subparsers.add_parser("list", help="List all files from other dumps in hash check compatible format")
    parser_dump.add_argument("directory", help="Directory of .gz dump files generated by 'compress'")
    parser_dump.add_argument("-o", metavar="output.txt", required=True, help="Output .txt filename")
    parser_dump.set_defaults(func=do_list)

    parser_diff = subparsers.add_parser("diff", help="Compare dumps")
    parser_diff.add_argument("directory", help="Directory of .gz dump files generated by 'compress'")
    parser_diff.add_argument("--ref", metavar="ref.gz", required=True, help="Reference hash dump .gz")
    parser_diff.set_defaults(func=do_diff)

    argv = parser.parse_args()
    if "func" not in argv:
        parser.print_help()
    else:
        argv.func(argv)
