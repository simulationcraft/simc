string_to_split = "foo"

splitted_len = 120

str_len = len(string_to_split)

num_splits = str_len / splitted_len
remainder = ( str_len % splitted_len ) > 0

for i in range(num_splits + remainder):
	print "\"" + string_to_split[i*splitted_len:(i+1)*splitted_len] + "\""
