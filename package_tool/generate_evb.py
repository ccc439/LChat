"""Generate Enigma Virtual Box .evb project file (UTF-16LE XML)."""
import sys
import os

def generate_evb(project_name, input_exe, output_exe, path_to_pack):
    input_exe = os.path.abspath(input_exe)
    output_exe = os.path.abspath(output_exe)
    path_to_pack = os.path.abspath(path_to_pack)

    def file_entry(name, full_path):
        return (
            f'<File>\n'
            f'\t<Type>2</Type>\n'
            f'\t<Name>{name}</Name>\n'
            f'\t<File>{full_path}</File>\n'
            f'\t<ActiveX>false</ActiveX>\n'
            f'\t<ActiveXInstall>false</ActiveXInstall>\n'
            f'\t<Action>0</Action>\n'
            f'\t<OverwriteDateTime>false</OverwriteDateTime>\n'
            f'\t<OverwriteAttributes>false</OverwriteAttributes>\n'
            f'\t<PassCommandLine>false</PassCommandLine>\n'
            f'</File>'
        )

    def dir_entry(name, children):
        return (
            f'<File>\n'
            f'\t<Type>3</Type>\n'
            f'\t<Name>{name}</Name>\n'
            f'\t<Action>0</Action>\n'
            f'\t<OverwriteDateTime>false</OverwriteDateTime>\n'
            f'\t<OverwriteAttributes>false</OverwriteAttributes>\n'
            f'\t<Files>{children}</Files>\n'
            f'</File>'
        )

    def walk_dir(path):
        parts = []
        try:
            entries = sorted(os.listdir(path))
        except PermissionError:
            return ""
        for name in entries:
            full = os.path.join(path, name)
            if os.path.isdir(full):
                children = walk_dir(full)
                parts.append(dir_entry(name, children))
            else:
                parts.append(file_entry(name, full))
        return ''.join(parts)

    files_xml = walk_dir(path_to_pack)

    xml = (
        '<?xml encoding="utf-16"?>\n'
        '<>\n'
        f'\t<InputFile>{input_exe}</InputFile>\n'
        f'\t<OutputFile>{output_exe}</OutputFile>\n'
        '\t<Files>\n'
        '\t\t<Enabled>true</Enabled>\n'
        '\t\t<DeleteExtractedOnExit>true</DeleteExtractedOnExit>\n'
        '\t\t<CompressFiles>true</CompressFiles>\n'
        '\t\t<Files>\n'
        '\t\t\t<File>\n'
        '\t\t\t\t<Type>3</Type>\n'
        '\t\t\t\t<Name>%DEFAULT FOLDER%</Name>\n'
        '\t\t\t\t<Action>0</Action>\n'
        '\t\t\t\t<OverwriteDateTime>false</OverwriteDateTime>\n'
        '\t\t\t\t<OverwriteAttributes>false</OverwriteAttributes>\n'
        f'\t\t\t\t<Files>{files_xml}</Files>\n'
        '\t\t\t</File>\n'
        '\t\t</Files>\n'
        '\t</Files>\n'
        '\t<Registries>\n'
        '\t\t<Enabled>false</Enabled>\n'
        '\t\t<Registries>\n'
        '\t\t\t<Registry><Type>1</Type><Virtual>true</Virtual><Name>Classes</Name><ValueType>0</ValueType><Value/><Registries/></Registry>\n'
        '\t\t\t<Registry><Type>1</Type><Virtual>true</Virtual><Name>User</Name><ValueType>0</ValueType><Value/><Registries/></Registry>\n'
        '\t\t\t<Registry><Type>1</Type><Virtual>true</Virtual><Name>Machine</Name><ValueType>0</ValueType><Value/><Registries/></Registry>\n'
        '\t\t\t<Registry><Type>1</Type><Virtual>true</Virtual><Name>Users</Name><ValueType>0</ValueType><Value/><Registries/></Registry>\n'
        '\t\t\t<Registry><Type>1</Type><Virtual>true</Virtual><Name>Config</Name><ValueType>0</ValueType><Value/><Registries/></Registry>\n'
        '\t\t</Registries>\n'
        '\t</Registries>\n'
        '\t<Packaging>\n'
        '\t\t<Enabled>false</Enabled>\n'
        '\t</Packaging>\n'
        '\t<Options>\n'
        '\t\t<ShareVirtualSystem>false</ShareVirtualSystem>\n'
        '\t\t<MapExecutableWithTemporaryFile>true</MapExecutableWithTemporaryFile>\n'
        '\t\t<AllowRunningOfVirtualExeFiles>true</AllowRunningOfVirtualExeFiles>\n'
        '\t</Options>\n'
        '</>'
    )

    os.makedirs(os.path.dirname(project_name) or '.', exist_ok=True)
    with open(project_name, 'w', encoding='utf-16-le') as f:
        f.write(xml)

    return project_name


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Usage: python generate_evb.py <output.evb> <input.exe> <output_exe> <folder_to_pack>')
        print('Example: python generate_evb.py LChat_pack.evb LChat.exe LChat_Portable.exe LChat_Package')
        sys.exit(1)

    evb_path = sys.argv[1]
    input_exe = sys.argv[2]
    output_exe = sys.argv[3]
    folder = sys.argv[4]

    generate_evb(evb_path, input_exe, output_exe, folder)
    print(f'Generated: {evb_path}')
