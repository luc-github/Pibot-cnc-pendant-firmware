#!/usr/bin/env python3
"""
Simplified script for LVGL image conversion
Uses configuration from image_config.py

Version: 2025-06-15_v4 - Fixed 0-byte file issue + better error handling
"""

import os
import sys
import shutil
import subprocess
import time
from pathlib import Path

# Import configuration
try:
    from image_config import IMAGES_CONFIG, BASE_PATHS
except ImportError:
    print("❌ Error: image_config.py not found")
    sys.exit(1)

def get_path(key: str) -> Path:
    """Get absolute path from configuration"""
    script_dir = Path(__file__).parent
    return script_dir / BASE_PATHS[key]

def ensure_dir(path: Path) -> None:
    """Create directory if it doesn't exist"""
    path.mkdir(parents=True, exist_ok=True)

def check_environment():
    """Check if environment is ready"""
    print("🔍 Checking environment...")
    
    # Check LVGL script
    lvgl_script = get_path('lvgl_script')
    if not lvgl_script.exists():
        print(f"❌ LVGLImage.py not found at: {lvgl_script}")
        print("   Make sure LVGL is properly installed")
        return False
    else:
        print(f"✅ LVGL script found: {lvgl_script}")
    
    # Check if we have any images configured
    if not IMAGES_CONFIG:
        print("❌ No images configured in IMAGES_CONFIG")
        print("   Edit image_config.py to add your images")
        return False
    else:
        print(f"✅ {len(IMAGES_CONFIG)} images configured")
    
    return True

def convert_and_copy(config, progress: str = ""):
    """Convert and copy an image"""
    print(f"{progress}🔄 {config.source_file} ({config.format_type})")
    
    # Source paths
    if config.source_dir not in BASE_PATHS:
        print(f"  ❌ Unknown source directory key: {config.source_dir}")
        return False
    
    source_dir = get_path(config.source_dir)
    source = source_dir / config.source_file
    lvgl_script = get_path('lvgl_script')
    
    print(f"  📂 Source dir: {source_dir}")
    print(f"  📄 Source file: {source}")
    
    # DEBUG: Show path components
    print(f"  🐛 DEBUG:")
    print(f"      source.name = '{source.name}'")
    print(f"      source.stem = '{source.stem}'")
    print(f"      source.suffix = '{source.suffix}'")
    print(f"      config.output_name = '{config.output_name}'")
    
    # Verify source file exists
    if not source.exists():
        print(f"  ❌ Source file not found: {source}")
        return False
    
    # Show file info
    file_size = source.stat().st_size
    print(f"  📊 Source: {source.name} ({file_size} bytes)")
    
    # Determine output file - Fix double extension issue
    if config.output_name:
        # Use custom name, ensure no double .c extension
        output_name = config.output_name
        if output_name.endswith('.c'):
            output_name = output_name[:-2]  # Remove .c if present
    else:
        # Use source filename without extension
        output_name = source.stem
    
    output_file = source_dir / f"{output_name}.c"
    
    print(f"  🎯 Output will be: {output_file}")
    
    # Remove previous version if exists
    if output_file.exists():
        try:
            output_file.unlink()
            print(f"  🗑️ Removed previous: {output_file.name}")
        except Exception as e:
            print(f"  ⚠️  Cannot remove previous version: {e}")
    
    # Build command - Use absolute paths to avoid confusion
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent
    
    # Use absolute paths for clarity
    abs_script = project_root / "components/lvgl/scripts/LVGLImage.py"
    abs_source = source  # Use actual source path
    
    # LVGLImage creates a directory with the output name and puts the file inside
    # So if we specify "alarm_s.c", it creates "alarm_s.c/alarm_s.c"
    output_name_for_lvgl = output_file.name  # e.g., "alarm_s.c"
    actual_output_dir = source_dir / output_name_for_lvgl  # The directory LVGL will create
    actual_output_file = actual_output_dir / output_name_for_lvgl  # The actual file location
    
    cmd = [
        sys.executable, str(abs_script),
        "-o", output_name_for_lvgl,  # LVGL will create this as a directory
        "--cf", config.format_type,
        "--ofmt", config.output_format or "C",
    ]
    
    # Add optional parameters
    if config.compression and config.compression != 'NONE':
        cmd.extend(["--compress", config.compression])
    
    if config.premultiply:
        cmd.append("--premultiply")
    
    # PNG file MUST be last
    cmd.append(str(abs_source))
    
    print(f"  🔧 Command: {' '.join(cmd)}")
    print(f"  📁 Working directory: {source_dir}")
    print(f"  🎯 LVGL will create: {actual_output_dir}/")
    print(f"  🎯 File will be at: {actual_output_file}")
    
    try:
        # Execute the command from the source directory
        result = subprocess.run(cmd, check=False, capture_output=True, text=True, 
                              cwd=source_dir, timeout=30)
        
        print(f"  📋 Return code: {result.returncode}")
        
        if result.stdout:
            print(f"  📤 STDOUT: {result.stdout.strip()}")
        if result.stderr:
            print(f"  📥 STDERR: {result.stderr.strip()}")
        
        if result.returncode != 0:
            print(f"  ❌ Conversion failed (exit code {result.returncode})")
            return False
        
        # Check if file was created and has content
        # LVGL creates: source_dir/alarm_s.c/alarm_s.c
        max_wait = 3
        for attempt in range(max_wait):
            if actual_output_file.exists():
                size = actual_output_file.stat().st_size
                if size > 0:
                    print(f"  ✅ Generated: {actual_output_file.relative_to(source_dir)} ({size} bytes)")
                    
                    # Copy the file to the expected location (source_dir/alarm_s.c)
                    # But since LVGL creates a directory, we keep using the file in the subdirectory
                    print(f"  📁 File ready at: {actual_output_file.relative_to(source_dir)}")
                    
                    # Update output_file to point to the actual location for the rest of the process
                    output_file = actual_output_file
                    
                    break
                else:
                    print(f"  ⏱️  Attempt {attempt + 1}: File exists but empty ({size} bytes)")
                    time.sleep(1)
            else:
                print(f"  ⏱️  Attempt {attempt + 1}: File not found at {actual_output_file}")
                time.sleep(1)
        else:
            print(f"  ❌ Output file was not created at expected location: {actual_output_file}")
            return False
        
    except subprocess.TimeoutExpired:
        print(f"  ❌ Conversion timed out after 30 seconds")
        return False
    except Exception as e:
        print(f"  ❌ Unexpected error during conversion: {e}")
        return False
    
    # Copy to targets
    success = True
    for target_key in config.target_dirs:
        if target_key not in BASE_PATHS:
            print(f"  ❌ Unknown target key: {target_key}")
            success = False
            continue
            
        target_dir = get_path(target_key)
        ensure_dir(target_dir)
        dest = target_dir / output_file.name
        
        try:
            if dest.exists():
                dest.unlink()
            shutil.copy2(output_file, dest)
            dest_size = dest.stat().st_size
            target_name = target_dir.name
            print(f"  📁 → {target_name}/{dest.name} ({dest_size} bytes)")
        except Exception as e:
            print(f"  ❌ Copy error to {target_key}: {e}")
            success = False
    
    return success

def main():
    """Main function"""
    print("🖼️  LVGL Conversion - Fixed version")
    print("=" * 50)
    
    # Environment check
    if not check_environment():
        print("\n❌ Environment check failed")
        return 1
    
    print()
    
    total = len(IMAGES_CONFIG)
    success = 0
    
    for i, config in enumerate(IMAGES_CONFIG, 1):
        progress = f"[{i:2d}/{total}] "
        if convert_and_copy(config, progress):
            success += 1
        print()
    
    # Summary
    print("=" * 50)
    print(f"✅ Success: {success}/{total}")
    
    if success == total:
        print("🎉 All images are ready!")
        return 0
    else:
        print(f"⚠️  {total - success} error(s)")
        print("\n💡 If you get 0-byte files, try:")
        print("   1. Check that the PNG file is valid")
        print("   2. Try a different color format (RGB565 instead of I4)")
        print("   3. Run the LVGL command manually to see detailed errors")
        return 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)
