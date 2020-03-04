import string

def toCComment(text):
    out = "/*\n"
    for line in text.splitlines():
        out += (" * " + line).rstrip(' ') + "\n"
    out += " */"
    return out