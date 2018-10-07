### Python load MNIST fileset test

def concat_bytes(bytes):
	newint = 0
	for byte in bytes:
		newint = newint | (0xFF & byte)
		newint = newint << 8
	newint = newint >> 8 # steps back once, loop goes over one too many
	return newint

filename = 'train-images.idx3-ubyte'

testimages = open(filename, 'rb').read()

datatypes = {
0x08: 'unsigned byte',
0x09: 'signed byte',
0x0b: 'short',
0x0c: 'integer',
0x0d: 'float',
0x0e: 'double' }

print(datatypes.get(testimages[2]))

dimsno = int(testimages[3])

if dimsno > 2:
	dimsno = 2
	print("found " + str(dimsno) + " dims, reducing to two")

print("number of images is: " + str(concat_bytes(testimages[4:8])))

dims = [0 for i in range(dimsno)]

for i in range(dimsno):
	dims[i] = concat_bytes(testimages[8 + 4*i:12 + 4*i])
	print("dim " + str(i + 1) + " is " + str(dims[i]))
	
imagecount = 0
matsize = 1
for dim in dims:
	matsize *= dim
print(16, len(testimages))

for i in range(16, len(testimages), matsize):
	imagecount += 1
		
#counter = (imagecount - 1) * matsize + 16
counter = 16;

for i in range(dims[0]):
	for j in range(dims[1]):
		if testimages[counter] > 0:
			print("#", end="")
		else:
			print(" ", end="")
		counter += 1
	print()
		
print(imagecount, counter)

	


