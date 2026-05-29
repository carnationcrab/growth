#!/usr/bin/env python3
"""One-off migration: replace #include <std...> and std:: aliases in sim_core."""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SIM_CORE = REPO / "gde" / "sim_core"
GDE_SRC = REPO / "gde" / "src"
GATEWAY = SIM_CORE / "include" / "base" / "gateway"

INCLUDE_MAP = {
    "algorithm": "base/gateway/Calgorithm.hpp",
    "array": "base/gateway/Carray.hpp",
    "cassert": "base/gateway/Cassert.hpp",
    "atomic": "base/gateway/Catomic.hpp",
    "cctype": "base/gateway/Ccctype.hpp",
    "chrono": "base/gateway/Cchrono.hpp",
    "condition_variable": "base/gateway/CconditionVariable.hpp",
    "cstdlib": "base/gateway/Ccstdlib.hpp",
    "cstring": "base/gateway/Ccstring.hpp",
    "ctime": "base/gateway/Cctime.hpp",
    "deque": "base/gateway/Cdeque.hpp",
    "fstream": "base/gateway/Cfstream.hpp",
    "functional": "base/gateway/Cfunctional.hpp",
    "iomanip": "base/gateway/Ciomanip.hpp",
    "iostream": "base/gateway/Ciostream.hpp",
    "limits": "base/gateway/Climits.hpp",
    "map": "base/gateway/Cmap.hpp",
    "cmath": "base/gateway/Cmath.hpp",
    "cstddef": "base/gateway/Cstddef.hpp",
    "cstdint": "base/gateway/Cstdint.hpp",
    "memory": "base/gateway/Cmemory.hpp",
    "mutex": "base/gateway/Cmutex.hpp",
    "optional": "base/gateway/Coptional.hpp",
    "queue": "base/gateway/Cqueue.hpp",
    "unordered_set": "base/gateway/Cunordered_set.hpp",
    "set": "base/gateway/Cset.hpp",
    "sstream": "base/gateway/Csstream.hpp",
    "string": "base/gateway/Cstring.hpp",
    "thread": "base/gateway/Cthread.hpp",
    "typeindex": "base/gateway/Ctypeindex.hpp",
    "unordered_map": "base/gateway/Cunordered_map.hpp",
    "utility": "base/gateway/Cutility.hpp",
    "variant": "base/gateway/Cvariant.hpp",
    "vector": "base/gateway/Cvector.hpp",
    "windows.h": "base/gateway/Cwin.hpp",
    "wincrypt.h": "base/gateway/Cwin.hpp",
    "fcntl.h": "base/gateway/Cposix.hpp",
    "unistd.h": "base/gateway/Cposix.hpp",
}

ANGLE_INCLUDE = re.compile(r'^\s*#include\s*<([^>]+)>\s*$')

REPLACEMENTS = [
    (r"std::unordered_map", "UnorderedMap"),
    (r"std::unique_lock", "Cmutex::UniqueLock"),
    (r"std::lock_guard", "Cmutex::LockGuard"),
    (r"std::make_unique", "Cmemory::make_unique"),
    (r"std::make_shared", "Cmemory::make_shared"),
    (r"std::unique_ptr", "UniquePtr"),
    (r"std::shared_ptr", "SharedPtr"),
    (r"std::weak_ptr", "Cmemory::WeakPtr"),
    (r"std::istringstream", "IStringStream"),
    (r"std::ostringstream", "OStringStream"),
    (r"std::stringstream", "StringStream"),
    (r"std::numeric_limits<([^>]+)>::infinity\(\)", r"Climits::NumericLimits<\1>::infinity()"),
    (r"std::numeric_limits<([^>]+)>::max\(\)", r"Climits::NumericLimits<\1>::max()"),
    (r"std::numeric_limits<([^>]+)>::min\(\)", r"Climits::NumericLimits<\1>::min()"),
    (r"std::numeric_limits<([^>]+)>::lowest\(\)", r"Climits::NumericLimits<\1>::lowest()"),
    (r"std::chrono::milliseconds", "Milliseconds"),
    (r"std::this_thread::yield", "Cthread::yield"),
    (r"std::type_index", "TypeIndex"),
    (r"std::optional", "Optional"),
    (r"std::variant", "Variant"),
    (r"std::function", "Function"),
    (r"std::vector", "Vector"),
    (r"std::string", "String"),
    (r"std::array", "Array"),
    (r"std::deque", "Deque"),
    (r"std::map", "Map"),
    (r"std::set", "Set"),
    (r"std::unordered_set", "UnorderedSet"),
    (r"std::queue", "Queue"),
    (r"std::pair", "Pair"),
    (r"std::atomic", "Catomic::Atomic"),
    (r"std::memcpy", "Ccstring::memcpy"),
    (r"std::stod", "Cstring::stod"),
    (r"std::getline", "getline"),
    (r"std::sort", "Calgorithm::sort"),
    (r"std::transform", "Calgorithm::transform"),
    (r"std::tolower", "Ccctype::tolower"),
    (r"std::move", "Cutility::move"),
    (r"std::forward", "Cutility::forward"),
    (r"std::malloc", "Ccstdlib::malloc"),
    (r"std::free", "Ccstdlib::free"),
    (r"std::cout", "Ciostream::cout()"),
    (r"std::cerr", "Ciostream::cerr()"),
    (r"std::endl", '"\\n"'),
    (r"std::fabs", "Cmath::abs"),
    (r"std::sqrt", "Cmath::sqrt"),
    (r"std::sin", "Cmath::sin"),
    (r"std::cos", "Cmath::cos"),
    (r"std::exp", "Cmath::exp"),
    (r"std::log", "Cmath::log"),
    (r"std::pow", "Cmath::pow"),
    (r"std::floor", "Cmath::floor"),
    (r"std::min", "Calgorithm::min"),
    (r"std::max", "Calgorithm::max"),
    (r"std::swap", "Cutility::swap"),
    (r"std::size_t", "Size"),
    (r"std::time", "Cctime::time"),
    (r"std::mutex", "Cmutex::Mutex"),
    (r"std::thread", "Cthread::Thread"),
    (r"std::condition_variable", "CconditionVariable::ConditionVariable"),
]


def wrapper_include_path(header: str, file_path: Path) -> str:
    rel = INCLUDE_MAP.get(header)
    if not rel:
        return ""
    return f'"{rel}"'


def migrate_file(path: Path, allow_godot: bool = False) -> bool:
    if path.is_relative_to(GATEWAY):
        return False
    try:
        text = path.read_text(encoding="utf-8")
    except OSError:
        return False

    original = text
    new_includes: list[str] = []
    lines_out: list[str] = []

    for line in text.splitlines():
        m = ANGLE_INCLUDE.match(line)
        if m:
            hdr = m.group(1)
            if allow_godot and (hdr.startswith("godot_cpp/") or hdr == "gdextension_interface.h"):
                lines_out.append(line)
                continue
            if hdr in INCLUDE_MAP:
                inc = wrapper_include_path(hdr, path)
                if inc and inc not in new_includes:
                    new_includes.append(inc)
                continue
            if hdr in ("windows.h", "wincrypt.h", "fcntl.h", "unistd.h"):
                inc = wrapper_include_path(hdr, path)
                if inc and inc not in new_includes:
                    new_includes.append(inc)
                continue
            print(f"WARN unmapped include <{hdr}> in {path}", file=sys.stderr)
            lines_out.append(line)
            continue
        lines_out.append(line)

    text = "\n".join(lines_out)
    if original.endswith("\n") and not text.endswith("\n"):
        text += "\n"

    for pattern, repl in REPLACEMENTS:
        text = re.sub(pattern, repl, text)

    if new_includes:
        insert_at = 0
        file_lines = text.splitlines()
        for i, line in enumerate(file_lines):
            if line.startswith("#include"):
                insert_at = i + 1
            elif line.strip() and not line.startswith("//") and insert_at > 0:
                break
        for inc in reversed(new_includes):
            inc_line = f"#include {inc}"
            if inc_line not in file_lines:
                file_lines.insert(insert_at, inc_line)
        text = "\n".join(file_lines)
        if original.endswith("\n"):
            text += "\n"

    if text != original:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> int:
    changed = 0
    for base in (SIM_CORE / "include", SIM_CORE / "src"):
        for path in base.rglob("*"):
            if path.suffix not in (".cpp", ".hpp", ".h"):
                continue
            if migrate_file(path):
                changed += 1
                print(path.relative_to(REPO))
    for path in GDE_SRC.rglob("*"):
        if path.suffix not in (".cpp", ".hpp", ".h"):
            continue
        if migrate_file(path, allow_godot=True):
            changed += 1
            print(path.relative_to(REPO))
    print(f"Updated {changed} file(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
