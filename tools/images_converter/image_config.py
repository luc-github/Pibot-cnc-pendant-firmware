"""
Image configuration for LVGL conversion
Separates configuration from processing code
"""

from dataclasses import dataclass
from typing import List, Optional

@dataclass
class ImageConfig:
    """Configuration for an image to convert"""
    source_dir: str         # Source directory key in BASE_PATHS
    source_file: str        # Source PNG filename (with extension)
    format_type: str        # Conversion type ('I4', 'I8', 'RGB565', etc.)
    target_dirs: List[str]  # List of target keys in BASE_PATHS
    output_name: Optional[str] = None       # Different output name (optional)
    output_format: Optional[str] = 'C'      # Output format: C, BIN, PNG (optional)
    compression: Optional[str] = 'NONE'     # Compression: NONE, RLE, LZ4 (optional)
    premultiply: bool = False               # Premultiply alpha (optional)

# =============================================================================
# PATH CONFIGURATION (relative from tools/images_converter/)
# =============================================================================

BASE_PATHS = {
    # LVGL script (go up 2 levels from tools/images_converter/)
    'lvgl_script': '../../components/lvgl/scripts/LVGLImage.py',
    
    # Source directories (by icon/theme) - output will be generated here directly
    'source_alarm': '../../resources/Alarm_s',         #Alarm status icon
    'source_home': '../../resources/Home_s',           #Home status icon      
    'source_settings': '../../resources/Settings_m',   # Settings menu icon
    'source_files': '../../resources/Files_m',         # Files menu icon
    'source_jog_s': '../../resources/Jog_s',           # Jog status icon
    'source_jog_m': '../../resources/Jog_m',           # Jog menu icon
    'source_probe': '../../resources/Probe_m',         # Probe menu icon
    'source_reset': '../../resources/Reset_m',         # Reset menu icon
    'source_info': '../../resources/Information_m',    # Information menu icon
    'source_positions': '../../resources/Positions_m', # Positions menu icon
    'source_macros': '../../resources/macros_m',       #  Macros menu icon
    'source_changetool': '../../resources/Changetool_m', #  Change tool menu icon
    'source_workspaces': '../../resources/Workspaces_m', #  Workspaces menu icon
    'source_logo': '../../resources/logo',             # Logo image
    'source_back_button': '../../resources/back_b',    #  Back button icon
    'source_change_b': '../../resources/change_b',    #  change tool button icon
    'source_check': '../../resources/Check_s',  # Check button icon
    'source_close_button': '../../resources/Close_b',  # Close button icon
    'source_door_s': '../../resources/Door_s',    # Door status icon
    'source_hold_s': '../../resources/Hold_s',    # Hold status icon
    'source_idle_s': '../../resources/Idle_s',    # Idle button icon
    'source_information': '../../resources/Information_m', # Information status icon
    'source_lock_button': '../../resources/Lock_b',    # Lock button icon
    'source_unlock_button': '../../resources/Unlock_b', # Unlock button icon  
    'source_macro_menu': '../../resources/macros_m',    # Macro menu icon
    'source_ok_button': '../../resources/Ok_b',        # Ok button icon
    'source_run_s': '../../resources/Run_s',      # Run status icon
    'source_sleep_button': '../../resources/Sleep_s',  # Sleep status icon
    'source_tools_s': '../../resources/Tools_s',  # Tools menu icon
    # Add your other source directories...
    
    # Target directories (by resolution and firmware)
    'target_grbl_320x240': '../../main/display/cnc/grblhal/res_320_240',
    'target_grbl_480x320': '../../main/display/cnc/grblhal/res_480_320',      # If exists
    'target_marlin_320x240': '../../main/display/cnc/marlin/res_320_240',     # If exists
    # Add your other targets...
}

# =============================================================================
# CONFIGURATION OF YOUR 30 IMAGES
# =============================================================================

IMAGES_CONFIG = [
    # ==== REAL EXAMPLE WITH YOUR DATA ====
    # Alarm - 24x24 icon from resources/Alarm_s/
    ImageConfig('source_alarm', 'alarm_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_home', 'home_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_settings', 'settings_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_files', 'files_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_jog_s', 'jog_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_jog_m', 'jog_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_probe', 'probe_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_reset', 'reset_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_info', 'information_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_positions', 'positions_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_macros', 'macros_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_changetool', 'changetool_m.png', 'I4', ['target_grbl_320x240']),  
    ImageConfig('source_workspaces', 'workspaces_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_logo', 'logo.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_back_button', 'back_b.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_change_b', 'change_b.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_check', 'check_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_close_button', 'close_b.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_door_s', 'door_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_hold_s', 'hold_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_idle_s', 'idle_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_information', 'information_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_lock_button', 'lock_b.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_unlock_button', 'unlock_b.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_macro_menu', 'macros_m.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_ok_button', 'ok_b.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_run_s', 'run_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_sleep_button', 'sleep_s.png', 'I4', ['target_grbl_320x240']),
    ImageConfig('source_tools_s', 'tools_s.png', 'I4', ['target_grbl_320x240']),
    # ==== EXAMPLES WITH ADVANCED OPTIONS ====
    # Uncomment and adapt when you create the files
    
    # # Home icon with RLE compression (good for simple icons)
    # ImageConfig('source_home', 'home_s.png', 'I4', ['target_grbl_320x240'], 
    #            compression='RLE'),
    
    # # Color logo with transparency
    # ImageConfig('source_logo', 'logo.png', 'ARGB8888', ['target_grbl_320x240'],
    #            premultiply=True),
    
    # # High-quality image with LZ4 compression
    # ImageConfig('source_background', 'bg.png', 'RGB565', ['target_grbl_320x240'],
    #            compression='LZ4'),
    
    # ==== TEMPLATE FOR YOUR OTHER ICONS ====
    # Basic examples - uncomment and adapt as needed
    
    # # Settings - icon from resources/Settings_s/
    # ImageConfig('source_settings', 'settings_s.png', 'I4', ['target_grbl_320x240']),
    
    # # Files - icon from resources/Files_s/
    # ImageConfig('source_files', 'files_s.png', 'I4', ['target_grbl_320x240']),
    
    # # Jog - icon from resources/Jog_s/
    # ImageConfig('source_jog', 'jog_s.png', 'I4', ['target_grbl_320x240']),
    
    # ==== CONTROL ICONS ====
    # ImageConfig('source_control', 'play_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_control', 'pause_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_control', 'stop_s.png', 'I4', ['target_grbl_320x240']),
    
    # ==== DIRECTIONAL ARROWS ====
    # ImageConfig('source_arrows', 'up_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_arrows', 'down_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_arrows', 'left_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_arrows', 'right_s.png', 'I4', ['target_grbl_320x240']),
    
    # ==== TOOLS ====
    # ImageConfig('source_tools', 'probe_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_tools', 'spindle_s.png', 'I4', ['target_grbl_320x240']),
    
    # ==== STATUS ====
    # ImageConfig('source_status', 'ok_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_status', 'warning_s.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_status', 'error_s.png', 'I4', ['target_grbl_320x240']),
    
    # ==== WITH CUSTOM NAMES ====
    # If you want a different output name
    # ImageConfig('source_alarm', 'alarm_icon.png', 'I4', ['target_grbl_320x240'], 
    #            output_name='alarm'),
    
    # ==== MULTI-RESOLUTIONS ====
    # For the same icon in multiple resolutions/firmwares
    # ImageConfig('source_home', 'home_320.png', 'I4', ['target_grbl_320x240']),
    # ImageConfig('source_home', 'home_480.png', 'I4', ['target_grbl_480x320']),
    # ImageConfig('source_home', 'home_320.png', 'I4', ['target_marlin_320x240']),
]

# =============================================================================
# SUPPORTED FORMATS (LVGL --cf parameter)
# =============================================================================

SUPPORTED_FORMATS = {
    # Luminance/Grayscale
    'L8': 'Luminance 8-bit (256 levels) - For grayscale images',
    
    # Indexed/Palette
    'I1': 'Indexed 1-bit (2 colors) - Minimal size',
    'I2': 'Indexed 2-bit (4 colors) - Very small',
    'I4': 'Indexed 4-bit (16 colors) - Recommended for simple icons',
    'I8': 'Indexed 8-bit (256 colors) - For complex icons',
    
    # Alpha only
    'A1': '1-bit alpha (2 levels) - For simple masks',
    'A2': '2-bit alpha (4 levels)',
    'A4': '4-bit alpha (16 levels)',
    'A8': '8-bit alpha (256 levels) - For complex masks',
    
    # RGB formats
    'RGB565': '16-bit color (65K colors) - Good for color images',
    'RGB565A8': '16-bit color + 8-bit alpha - Color with transparency',
    'ARGB8565': '16-bit color with 8-bit alpha - Alternative format',
    'RGB888': '24-bit color (16M colors) - High quality',
    'ARGB8888': '32-bit color + alpha - Full quality with transparency',
    'XRGB8888': '32-bit color without alpha - Full quality',
    
    # Special
    'AUTO': 'Auto-detect best format - Let LVGL choose',
    'RAW': 'Raw format - No conversion',
    'RAW_ALPHA': 'Raw format with alpha'
}

# =============================================================================
# OUTPUT FORMATS (LVGL --ofmt parameter)  
# =============================================================================

OUTPUT_FORMATS = {
    'C': 'C array (default) - For embedding in code',
    'BIN': 'Binary format - For external storage',
    'PNG': 'PNG format - For testing/preview'
}

# =============================================================================
# COMPRESSION OPTIONS (LVGL --compress parameter)
# =============================================================================

COMPRESSION_OPTIONS = {
    'NONE': 'No compression (default)',
    'RLE': 'Run-Length Encoding - Good for simple images',
    'LZ4': 'LZ4 compression - Better for complex images'
}

# =============================================================================
# HELPER FUNCTIONS
# =============================================================================

def print_config_summary():
    """Display configuration summary"""
    print("📋 CONFIGURATION SUMMARY")
    print("=" * 50)
    print(f"Number of images: {len(IMAGES_CONFIG)}")
    
    # Count by format
    format_count = {}
    for config in IMAGES_CONFIG:
        format_count[config.format_type] = format_count.get(config.format_type, 0) + 1
    
    print("\nFormats used:")
    for fmt, count in format_count.items():
        print(f"  {fmt}: {count} images")
    
    # Count by target
    target_count = {}
    for config in IMAGES_CONFIG:
        for target in config.target_dirs:
            target_count[target] = target_count.get(target, 0) + 1
    
    print("\nDistribution by resolution:")
    for target, count in target_count.items():
        print(f"  {target}: {count} images")

def validate_config():
    """Validate configuration"""
    errors = []
    
    for i, config in enumerate(IMAGES_CONFIG):
        # Check extension
        if not config.source_file.lower().endswith('.png'):
            errors.append(f"Image {i+1}: {config.source_file} is not a PNG")
        
        # Check format
        if config.format_type not in SUPPORTED_FORMATS:
            errors.append(f"Image {i+1}: Format {config.format_type} not supported")
        
        # Check targets
        valid_targets = [key for key in BASE_PATHS.keys() if key.startswith('target_')]
        for target in config.target_dirs:
            if target not in valid_targets:
                errors.append(f"Image {i+1}: Target {target} not valid")
    
    return errors

if __name__ == "__main__":
    print_config_summary()
    
    errors = validate_config()
    if errors:
        print("\n❌ CONFIGURATION ERRORS:")
        for error in errors:
            print(f"  - {error}")
    else:
        print("\n✅ Configuration is valid!")
