import textwrap
from os import PathLike
from tempfile import NamedTemporaryFile
import dataclasses
import re
import subprocess
from pathlib import Path


@dataclasses.dataclass
class DriverInfo:
    peripherals: list[str]
    src: list[str]

    def to_cmake(self, basename: str) -> str:
        name = f"mat91lib_{basename}"
        lines = [
            f"add_library({name} INTERFACE)",
            f"add_library(mat91lib::{basename} ALIAS {name})",
            f"target_sources({name} INTERFACE",
            *(f"    {src}" for src in self.src),
            ")",
            "target_include_directories(",
            f"    {name}",
            "    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}",
            ")",
        ]

        libraries = {f"mat91lib::{lib}" for lib in self.peripherals}
        if libraries:
            lines.append(f"target_link_libraries({name} INTERFACE")
            lines.extend(f"    {lib}" for lib in sorted(libraries))
            lines.append(")")

        return "\n".join(lines)


def parse_make_variable_dump(line: str) -> tuple[str, str]:
    match = re.match(r"^(\w+) :?= (.*)$", line)
    if not match:
        msg = f"invalid variable dump: {line}"
        raise ValueError(msg)

    return match[1], match[2]


def get_makefile_variables(path: PathLike | str) -> dict[str, str] | None:
    with NamedTemporaryFile("w", delete_on_close=False) as tempfile:
        # Add a dummy rule to avoid "no targets error"
        tempfile.write("all:\n\n")
        tempfile.write(Path(path).read_text())
        tempfile.close()

        r = subprocess.run(
            (
                "make",
                "--file",
                tempfile.name,
                "--dry-run",
                "--silent",
                "--print-data-base",
            ),
            stdout=subprocess.PIPE,
            text=True,
        )
        if r.returncode:
            print(f"Warning: error parsing makefile {path}, skipping...")
            return None

    vars: dict[str, str] = {}
    lines = iter(r.stdout.splitlines())
    line = next(lines, None)
    while line is not None:
        if line.startswith("# makefile ("):
            # next line should be a variable declaration:
            line = next(lines)
            var, val = parse_make_variable_dump(line)
            vars[var] = val

        line = next(lines, None)

    return vars


def parse_makefile(path: PathLike | str) -> DriverInfo | None:
    vars = get_makefile_variables(path)
    if not vars:
        return None

    src = vars.get("SRC")
    if not src:
        print(f"Makefile missing SRC: {path}. Skipping...")
        return None

    return DriverInfo(
        src=src.split(),
        peripherals=vars.get("PERIPHERALS", "").split(),
    )


_cmake_main_library_setup = """\
include(${CMAKE_CURRENT_LIST_DIR}/cmake/utils.cmake)

add_subdirectory(sam4s)
if(DEFINED MCU)
    get_mcu_family(${MCU} family)
    set(FAMILY ${family} PARENT_SCOPE)
    message(STATUS "Configuring mat91lib for ${MCU} (family: ${family})")

    string(TOLOWER ${family} family_lower)
    set(family_dir ${CMAKE_CURRENT_SOURCE_DIR}/${family_lower})
    set(MAT91LIB_FAMILY_DIR ${family_dir} PARENT_SCOPE)

    add_library(mat91lib INTERFACE)
    target_include_directories(
        mat91lib INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${family_lower}
    )
    target_compile_definitions(mat91lib INTERFACE __${MCU}__ __${family}__)
    target_link_libraries(mat91lib INTERFACE mat91lib::${family_lower})
    target_link_options(mat91lib INTERFACE -L${family_dir} -T${MCU}-ROM.ld)
    target_compile_options(mat91lib INTERFACE
        # Prevent "ISO C++17 does not allow 'register' storage class specifier"
        # warning from causing error:
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=register>
    )

    # Chip-specific peripheral libraries:
    add_library(mat91lib::pit ALIAS mat91lib_${family_lower}_pit)
    add_library(mat91lib::sysclock ALIAS mat91lib_${family_lower}_sysclock)
    add_library(mat91lib::systick ALIAS mat91lib_${family_lower}_systick)
else()
    message(
        WARNING
        "MCU not defined - mat91lib core libraries won't be available."
    )
endif()
"""


def main() -> None:
    libs: list[str] = []
    for dir in (path for path in Path().iterdir() if path.is_dir()):
        if dir.name in ("sam4s", "sam7s"):
            print(f"Skipping {dir}...")
            continue

        mk_paths = list(dir.glob("*.mk"))
        makefile_path = dir / "Makefile"
        if makefile_path.exists():
            mk_paths.append(makefile_path)

        if not mk_paths:
            continue

        if len(mk_paths) > 1:
            raise ValueError(f"found multiple makefiles: {mk_paths}")

        cmake_path = dir / "CMakeLists.txt"

        makefile_path = mk_paths[0]
        if makefile_path.name == "Makefile":
            lib_name = dir.name
        else:
            lib_name = makefile_path.stem

        lib_info = parse_makefile(makefile_path)
        if lib_info:
            libs.append(lib_name)
            cmake_src = lib_info.to_cmake(lib_name)
            with cmake_path.open("w") as f:
                f.write(cmake_src)
                f.write("\n")

            print(f"Created {cmake_path}")

    libs.sort()
    with open("CMakeLists.txt", "w") as f:
        f.write(textwrap.dedent("""\
        cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

        project(mat91lib LANGUAGES C)

        """))
        f.write(_cmake_main_library_setup)
        f.write("\n")

        for lib in libs:
            f.write(f"add_subdirectory({lib} EXCLUDE_FROM_ALL)\n")


if __name__ == "__main__":
    main()
