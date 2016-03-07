import sys
import re
import json

def get_file_as_lines( input, out_line_length ):
    out = []
    with open( input ) as f:
        str = f.read()
        str = re.sub( "\n", "", str )
        str = re.sub( "\t", "", str )
        str = re.sub( "\s+", " ", str )
        str = json.dumps(str).strip('"') # Misuse to build a C string with escaped characters
        eaten_chars = 0
        out = []
        while eaten_chars < len(str):
            total_line_len = out_line_length
            if eaten_chars + out_line_length > len(str):
                total_line_len = len(str) - eaten_chars
                out_line_length = total_line_len
            while str[eaten_chars + out_line_length - (total_line_len - out_line_length) - 1] == '\\':
                total_line_len += 1
            out.append(str[eaten_chars:eaten_chars + total_line_len])
            eaten_chars += total_line_len
    return (input, out)
            
def print_as_char_array( name, input ):
    ( filename, input_list) = input
    out = ("// Automatically generated from file %s\n"%(filename))
    out += ("static const char* %s[] = {\n"%(name))
    for line in input_list:
        out += "\"%s\",\n"%(line)
    out += "};";
    return out
        
def main():
    with open( "report_data.inc", "w") as f:
        f.write( "// Automatically generated report data.\n\n")
        f.write( print_as_char_array( "__logo", get_file_as_lines( "simc_logo.txt", 80 ) ) )
        f.write( "\n\n" )
        f.write( print_as_char_array( "__image_load_script", get_file_as_lines( "image_load_script.js", 80 ) ) )
        f.write( "\n\n" )
        f.write( print_as_char_array( "__mop_stylesheet", get_file_as_lines( "mop_style.css", 80 ) ) )
        f.write( "\n\n" )
        f.write( print_as_char_array( "__highcharts_include", get_file_as_lines( "highcharts.inc", 80 ) ) )
        f.write( "\n\n" )
        f.write( print_as_char_array( "__jquery_include", get_file_as_lines( "jquery-1.12.1.min.js", 80 ) ) )
    print( "done")
    
if __name__ == "__main__":
    main()
         
