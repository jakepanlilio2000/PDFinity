import os, sys, subprocess, shutil

def get_vcvars_env(arch="x64"):
    candidates = [
        r"C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
    ]
    bat = None
    for c in candidates:
        if os.path.exists(c):
            bat = c
            break
    if not bat:
        raise RuntimeError("VsDevCmd.bat not found in standard Visual Studio locations")
    cmd = f'"{bat}" -arch={arch} && set'
    out = subprocess.check_output(f'cmd.exe /c "{cmd}"', shell=True, text=True)
    env = {}
    for line in out.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            env[k] = v
    # Windows environment names are case-insensitive, unlike Python dict keys.
    # Preserve the developer PATH even when cmd spells it "Path".
    path_keys = [key for key in env if key.casefold() == 'path']
    if not path_keys or not env[path_keys[0]]:
        raise RuntimeError("MSVC developer environment did not provide PATH")
    native_path = env[path_keys[0]]
    for key in path_keys:
        del env[key]
    env['PATH'] = native_path
    # Add the declared local Qt runtime only when it is actually installed.
    qt_bin = r"C:\Qt\6.8.2\msvc2022_64\bin"
    if os.path.isdir(qt_bin):
        env['PATH'] = qt_bin + ';' + native_path
    return env

def run_command(cmd_str, env, cwd=None):
    print(f"\n>>> Running: {cmd_str}")
    res = subprocess.run(f"cmd.exe /c \"{cmd_str}\"", env=env, cwd=cwd, capture_output=True, text=True, shell=True)
    if res.stdout:
        print("--- STDOUT ---")
        print(res.stdout)
    if res.stderr:
        print("--- STDERR ---")
        print(res.stderr)
    print(f"--- EXIT CODE: {res.returncode} ---")
    return res.returncode

def main():
    env = get_vcvars_env("x64")
    
    # 1. CMake Configure
    rc = run_command("cmake --preset windows-x64-release", env=env)
    if rc != 0:
        sys.exit(rc)
        
    # 2. CMake Build
    rc = run_command("cmake --build --preset windows-x64-release", env=env)
    if rc != 0:
        sys.exit(rc)
        
    # 3. CTest
    rc = run_command("ctest --preset windows-x64-release", env=env)
    if rc != 0:
        sys.exit(rc)
        
    # 4. Binary execution test: --version
    exe = os.path.abspath(r"build\windows-x64-release\bin\PDFinity.exe")
    rc = run_command(f'"{exe}" --version', env=env)
    if rc != 0:
        sys.exit(rc)
        
    # 5. Binary execution test: --help
    rc = run_command(f'"{exe}" --help', env=env)
    if rc != 0:
        sys.exit(rc)
        
    # 6. Binary execution test: --headless-smoke
    rc = run_command(f'"{exe}" --headless-smoke', env=env)
    if rc != 0:
        sys.exit(rc)

    print("\n==========================================")
    print("ALL BUILD, INTEGRATION, AND SMOKE CHECKS PASSED!")
    print("==========================================")

if __name__ == "__main__":
    main()
