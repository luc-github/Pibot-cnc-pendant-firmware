import os
import re
import sys
import fnmatch
import argparse

def natural_sort_key(s):
    """
    Key for alphanumeric sorting.
    """
    return [int(c) if c.isdigit() else c.lower() for c in re.split(r'(\d+)', s)]

def get_file_priority(filename):
    """
    Determines the priority of a file according to specified rules.
    
    Order:
    1. CMakeLists.txt
    2. Makefile
    3. .h or .hpp files
    4. .c files
    5. .cpp files
    6. Other files (sorted by extension)
    
    Returns a tuple (priority_group, extension) for consistent sorting
    """
    base_name = os.path.basename(filename)
    ext = os.path.splitext(base_name)[1].lower()
    
    if base_name == "CMakeLists.txt":
        return (0, "")
    elif base_name == "Makefile":
        return (1, "")
    elif ext in ['.h', '.hpp']:
        return (2, ext)
    elif ext == '.c':
        return (3, ext)
    elif ext == '.cpp':
        return (4, ext)
    else:
        # For other extensions, we sort by their extension 
        return (5, ext)
    
def sort_entries(entries, directory):
    """
    Sorts entries according to specified rules.
    """
    # Separate directories and files
    dirs = [e for e in entries if os.path.isdir(os.path.join(directory, e))]
    files = [e for e in entries if not os.path.isdir(os.path.join(directory, e))]
    
    # Sort directories alphanumerically
    dirs.sort(key=natural_sort_key)
    
    # Sort files according to priorities and alphanumerically
    files.sort(key=lambda f: (get_file_priority(f), natural_sort_key(f)))
    
    # Return files first, then directories
    return files + dirs

def should_exclude(path, base_path, exclude_patterns):
    """
    Checks if a path should be excluded based on the exclusion patterns.
    
    Args:
        path: Path to check
        base_path: Base directory path for relative paths
        exclude_patterns: List of patterns to exclude
    
    Returns:
        True if path should be excluded, False otherwise
    """
    # Convert to absolute paths for comparison
    abs_path = os.path.abspath(path)
    abs_base_path = os.path.abspath(base_path)
    
    # Get relative path from base
    rel_path = os.path.relpath(abs_path, abs_base_path)
    rel_path = rel_path.replace('\\', '/')  # Normalize path separators
    
    # File or directory name
    name = os.path.basename(path)
    
    for pattern in exclude_patterns:
        # Case 1: Path starting with "/" - relative to base directory
        if pattern.startswith('/'):
            clean_pattern = pattern[1:]  # Remove leading '/'
            if rel_path == clean_pattern or rel_path.startswith(clean_pattern + '/'):
                return True
        
        # Case 2: Wildcard patterns with "**"
        elif '**' in pattern:
            # Replace ** with a regex pattern for matching any part of the path
            regex_pattern = pattern.replace('**', '.*')
            if re.match(f".*{regex_pattern}$", rel_path) or re.match(f".*{regex_pattern}$", name):
                return True
        
        # Case 3: Simple name match (anywhere in the path)
        else:
            if name == pattern:
                return True
    
    return False

def display_tree(directory, prefix="", is_last=False, base_path="", exclude_patterns=None):
    """
    Displays the directory tree structure.
    
    Args:
        directory: Path of the directory to explore
        prefix: Prefix used for indentation
        is_last: Indicates if this is the last item in the current directory
        base_path: Base directory path for relative paths
        exclude_patterns: List of patterns to exclude
    """
    if exclude_patterns is None:
        exclude_patterns = []
    
    # Skip if this directory should be excluded
    if directory != base_path and should_exclude(directory, base_path, exclude_patterns):
        return
    
    # Get the display name
    if directory == ".":
        display_name = os.path.basename(os.path.abspath("."))
    else:
        display_name = os.path.basename(directory) or directory
    
    # Determine the prefix to use for this item
    branch = "└── " if is_last else "├── "
    
    # Display the directory name
    print(f"{prefix}{branch}{display_name}")
    
    # Determine the new prefix for child elements
    new_prefix = prefix + ("    " if is_last else "│   ")
    
    # List all directory elements
    try:
        entries = os.listdir(directory)
        # Ignore hidden files
        entries = [e for e in entries if not e.startswith('.')]
        
        # Apply exclusion patterns
        filtered_entries = []
        for entry in entries:
            path = os.path.join(directory, entry)
            if not should_exclude(path, base_path, exclude_patterns):
                filtered_entries.append(entry)
        
        # Sort entries according to our custom rules
        sorted_entries = sort_entries(filtered_entries, directory)
        
        # For each element in the directory
        for i, entry in enumerate(sorted_entries):
            path = os.path.join(directory, entry)
            is_last_entry = i == len(sorted_entries) - 1
            
            # If it's a directory, recursive call
            if os.path.isdir(path):
                display_tree(path, new_prefix, is_last_entry, base_path, exclude_patterns)
            else:
                # Otherwise, it's a file, simply display it
                print(f"{new_prefix}{'└── ' if is_last_entry else '├── '}{entry}")
    except PermissionError:
        print(f"{new_prefix}Error: Permission denied")
    except OSError as e:
        print(f"{new_prefix}Error: {str(e)}")

def display_directory_tree(start_path, exclude_patterns=None):
    """
    Main entry point to display a directory tree structure.
    
    Args:
        start_path: Path of the directory to explore
        exclude_patterns: List of patterns to exclude
    """
    if exclude_patterns is None:
        exclude_patterns = []
    
    # Handle the current directory case
    actual_path = os.path.abspath(start_path)
    
    if os.path.exists(actual_path):
        if os.path.isdir(actual_path):
            # Get directory name for display
            if start_path == ".":
                dir_name = os.path.basename(actual_path)
                print(f"Tree structure of current directory ({dir_name}):")
            else:
                parent_dir = os.path.dirname(actual_path)
                if parent_dir:
                    print(f"Tree structure of {os.path.basename(actual_path)}:")
                else:
                    print(f"Tree structure of {actual_path}:")
            
            # Display the complete tree structure
            display_tree(start_path, "", True, actual_path, exclude_patterns)
        else:
            print(f"Error: {start_path} is not a directory.")
    else:
        print(f"Error: {start_path} does not exist.")

if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Display directory tree structure with custom sorting and filtering.')
    parser.add_argument('path', nargs='?', default='.', help='Directory path to display tree for (default: current directory)')
    parser.add_argument('-e', '--exclude', action='append', default=[], help='Patterns to exclude. Can be specified multiple times. Format: "/path" for root relative, "name" for anywhere, "**.ext" for wildcards')
    
    # Parse arguments
    args = parser.parse_args()
    
    # Display tree
    display_directory_tree(args.path, args.exclude)
    
    # Print usage if no arguments were provided
    if len(sys.argv) == 1:
        print("\nUsage examples:")
        print("  python tree.py                              # Display tree for current directory")
        print("  python tree.py /path/to/dir                 # Display tree for specified directory")
        print("  python tree.py -e /build -e components      # Exclude /build from root and any 'components' anywhere")
        print("  python tree.py -e **.lock                   # Exclude all .lock files")
