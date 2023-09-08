import sys
import getopt
import os
import shutil

# Add spec names here; these will be used to find the method in the apl file so should match your method name not necessarily tokenized spec name
# The script will only attempt to generate the APL of specs listed here
specList = ['blood', 'frost', 'unholy',  # Death Knight
            'arcane', 'fire', # Mage (frost is ignored because of duplicate name)
            'marksmanship', 'beast_mastery', 'survival', # Hunter
            'affliction', 'demonology', 'destruction', # Warlock
            'devastation', # Evoker
            'shadow', 'holy', 'discipline', # Priest
            'retribution', 'protection', # Paladin
            'assassination', 'outlaw', 'subtlety', 'assassination_df', 'outlaw_df', 'subtlety_df', # Rogue
            'havoc', 'vengeance', # Demon Hunter
            'protection', 'fury', 'arms', # Warrior
           ]

ignored_comments = [
    '# This default action priority list is automatically created based on your character.',
    '# It is a attempt to provide you with a action list that is both simple and practicable,',
    '# while resulting in a meaningful and good simulation. It may not result in the absolutely highest possible dps.',
    '# Feel free to edit, adapt and improve it to your own needs.',
    '# SimulationCraft is always looking for updates and improvements to the default action lists.',
    '# Executed before combat begins. Accepts non-harmful actions only.',
    '# Executed every time the actor is available.',
]

# Returns the next SimC string/line in file (defined as a comment line or TCI syntax to next whitespace)
def read_by_whitespace(fileObj):
    for line in fileObj:
        if line[0] == "#": # Yield whole line if starts with a comment character as comment lines can have whitespace
            if line.strip() not in ignored_comments:
                yield line
        else:
            for token in line.split(): # If it doesn't then just yield until next whitespace as TCI commands count them as line breaks
                yield token

# Opens the input file and generates C++ syntax apl from the given TCI syntax APL. List of sub-action lists returned in subaplList and list of actions (with comments) returned in aplList. aplList is formatted already.
def read_apl(inputFilePath):
    commentString = ""
    aplList = []
    subaplList = ["default"]

    with open(inputFilePath) as inputFile:
        for token in read_by_whitespace(inputFile):
            sublist = ""
            if "actions" in token and token[0] != '#': #Should this whole section be more robust? It should catch any legit comment or action line however will not error gracefully with improper inputs. Consider refactor if others end up using.
                action = token.replace("\"", "").split('=',1)
                if token[7] == ".":
                    if action[0][-1] == "+":
                        sublist = action[0][8:-1]
                    else:
                        sublist = action[0][8:]
                else:
                    sublist="default"
                if sublist not in subaplList:
                    subaplList.append(sublist)
                if sublist == "default":
                    sublist = "default_"
                if action[1][0] == "/":
                    action [1] = action[1][1:]
                if commentString == "":
                    aplList.append(f"  {sublist}->add_action( \"{action[1]}\" );")
                else:
                    aplList.append(f"  {sublist}->add_action( \"{action[1]}\", \"{commentString.strip()}\" );")
                commentString = ""
            else:
                commentString = commentString + " " + token[1:-1]
    return subaplList, aplList

# Replaces the existing outputFile with tempFile
def replace_file(outputFile, tempFile):
    outputFilePath = ""
    tempFilePath = ""
    try:
        outputFilePath = os.path.realpath(outputFile.name)
        tempFilePath = os.path.realpath(tempFile.name)
        tempFile.close()
        outputFile.close()
        shutil.move(tempFilePath, outputFilePath)
    except Exception as e:
        print(e)
        sys.exit(2)

# Write the c++ spec apl method to file
def write_apl_method (tempFile, specString, subaplList, aplList):
    tempFile.write(f"void {specString}( player_t* p )\n{{\n")
    for sublist in subaplList:
        if (sublist == "default"):
            tempFile.write(f"  action_priority_list_t* {sublist}_ = p->get_action_priority_list( \"{sublist}\" );\n")
        else:
            tempFile.write(f"  action_priority_list_t* {sublist} = p->get_action_priority_list( \"{sublist}\" );\n")
    tempFile.write("\n")
    current_list = aplList[0].split('->', 1)[0]
    for action in aplList:
        if not action.startswith(current_list + '->'):
            tempFile.write("\n")
            current_list = action.split('->', 1)[0]
        tempFile.write(action + "\n")
    tempFile.write("}")

# Usage: 'ConvertAPL.py -i <inputfile> -o <outputfile> -s <specstring>'
def main(argv):
    inputFilePath = ''
    outputFilePath = ''
    specString = ''
    try:
        opts, args = getopt.getopt(argv,"hi:o:s:",["input=","output=","spec="])
    except getopt.GetoptError:
        print ('ConvertAPL.py -i <inputfile> -o <outputfile> -s <specstring>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print ('ConvertAPL.py -i <inputfile> -o <outputfile> -s <specstring>')
            sys.exit()
        elif opt in ("-i", "--input"):
            inputFilePath = arg
        elif opt in ("-o", "--output"):
            outputFilePath = arg
        elif opt in ("-s", "--spec"):
            specString = arg.lower()
            if specString not in specList:
                print('Invalid spec selected')
                sys.exit(2)
    subaplList, aplList = read_apl(inputFilePath)

    with open(outputFilePath) as outputFile, open('tempFile.txt', 'w') as tempFile:
        with tempFile:
            changeMade = False
            inExistingMethod = False
            for line in outputFile:
                if not inExistingMethod:
                    if f"//{specString}_apl_start" in line:
                        inExistingMethod = True
                        changeMade = True
                        tempFile.write(f"//{specString}_apl_start\n")
                    else:
                        tempFile.write(line)
                else:
                    if f"//{specString}_apl_end" in line:
                        inExistingMethod = False
                        write_apl_method(tempFile, specString, subaplList, aplList)
                        tempFile.write(f"\n//{specString}_apl_end\n")
            if changeMade == True:
                replace_file(outputFile, tempFile)
            else:
                print('Method to replace not found or no changes to output file. Please check your file skeleton and spec names are correct')

if __name__ == "__main__":
    main(sys.argv[1:])
