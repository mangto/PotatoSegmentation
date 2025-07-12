import os

target_extensions = ['.c', '.h']
exception = ["stb_image.h"]

def is_valid_file(name:str) -> bool:
    
    isvalid = False

    if (name in exception): return False

    for ext in target_extensions:
        if (name.endswith(ext)): isvalid = True

    return isvalid


if __name__ == "__main__":
    lst = [name for name in os.listdir(".") 
                if is_valid_file(name)]
    
    buffer:str
    result = ''

    for name in lst:
        with open(f".\\{name}", 'r') as file:
            buffer = file.read()

        result += f"```{name}\n{buffer}\n```\n"
    
    with open(".\\EntireCode.txt", 'w') as file:
        file.write(result)