a = { 6, s = "hello world", ["subtable"] = { 1, 2, 3, key = "value"} }
b = a["subtable"].key
c = a.subtable.key
print(b, c)