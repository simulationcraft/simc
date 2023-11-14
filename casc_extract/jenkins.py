# vim: tabstop=4 shiftwidth=4 softtabstop=4

# Jenkins Hash implementation for Python.
# http://stackoverflow.com/questions/3279615/python-implementation-of-jenkins-hash
#
# Need to constrain U32 to only 32 bits using the & 0xffffffff
# since Python has no native notion of integers limited to 32 bit
# http://docs.python.org/library/stdtypes.html#numeric-types-int-float-long-complex

'''Original copyright notice:
	By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
	code any way you wish, private, educational, or commercial.  Its free.
'''
def rot(x,k):
	return (((x)<<(k)) | ((x)>>(32-(k))))

def mix(a, b, c):
	a &= 0xffffffff; b &= 0xffffffff; c &= 0xffffffff
	a -= c; a &= 0xffffffff; a ^= rot(c,4);  a &= 0xffffffff; c += b; c &= 0xffffffff
	b -= a; b &= 0xffffffff; b ^= rot(a,6);  b &= 0xffffffff; a += c; a &= 0xffffffff
	c -= b; c &= 0xffffffff; c ^= rot(b,8);  c &= 0xffffffff; b += a; b &= 0xffffffff
	a -= c; a &= 0xffffffff; a ^= rot(c,16); a &= 0xffffffff; c += b; c &= 0xffffffff
	b -= a; b &= 0xffffffff; b ^= rot(a,19); b &= 0xffffffff; a += c; a &= 0xffffffff
	c -= b; c &= 0xffffffff; c ^= rot(b,4);  c &= 0xffffffff; b += a; b &= 0xffffffff
	return a, b, c

def final(a, b, c):
	a &= 0xffffffff; b &= 0xffffffff; c &= 0xffffffff
	c ^= b; c &= 0xffffffff; c -= rot(b,14); c &= 0xffffffff
	a ^= c; a &= 0xffffffff; a -= rot(c,11); a &= 0xffffffff
	b ^= a; b &= 0xffffffff; b -= rot(a,25); b &= 0xffffffff
	c ^= b; c &= 0xffffffff; c -= rot(b,16); c &= 0xffffffff
	a ^= c; a &= 0xffffffff; a -= rot(c,4);  a &= 0xffffffff
	b ^= a; b &= 0xffffffff; b -= rot(a,14); b &= 0xffffffff
	c ^= b; c &= 0xffffffff; c -= rot(b,24); c &= 0xffffffff
	return a, b, c

def hashlittle2(data, initval = 0, initval2 = 0):
	length = lenpos = len(data)

	a = b = c = (0xdeadbeef + (length) + initval)

	c += initval2; c &= 0xffffffff

	p = 0  # string offset
	while lenpos > 12:
		a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24)); a &= 0xffffffff
		b += (data[p+4] + (data[p+5]<<8) + (data[p+6]<<16) + (data[p+7]<<24)); b &= 0xffffffff
		c += (data[p+8] + (data[p+9]<<8) + (data[p+10]<<16) + (data[p+11]<<24)); c &= 0xffffffff
		a, b, c = mix(a, b, c)
		p += 12
		lenpos -= 12

	if lenpos == 12: c += (data[p+8] + (data[p+9]<<8) + (data[p+10]<<16) + (data[p+11]<<24)); b += (data[p+4] + (data[p+5]<<8) + (data[p+6]<<16) + (data[p+7]<<24)); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24));
	if lenpos == 11: c += (data[p+8] + (data[p+9]<<8) + (data[p+10]<<16)); b += (data[p+4] + (data[p+5]<<8) + (data[p+6]<<16) + (data[p+7]<<24)); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24));
	if lenpos == 10: c += (data[p+8] + (data[p+9]<<8)); b += (data[p+4] + (data[p+5]<<8) + (data[p+6]<<16) + (data[p+7]<<24)); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24));
	if lenpos == 9:  c += (data[p+8]); b += (data[p+4] + (data[p+5]<<8) + (data[p+6]<<16) + (data[p+7]<<24)); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24));
	if lenpos == 8:  b += (data[p+4] + (data[p+5]<<8) + (data[p+6]<<16) + (data[p+7]<<24)); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24));
	if lenpos == 7:  b += (data[p+4] + (data[p+5]<<8) + (data[p+6]<<16)); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24));
	if lenpos == 6:  b += ((data[p+5]<<8) + data[p+4]); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24))
	if lenpos == 5:  b += (data[p+4]); a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24));
	if lenpos == 4:  a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16) + (data[p+3]<<24))
	if lenpos == 3:  a += (data[p+0] + (data[p+1]<<8) + (data[p+2]<<16))
	if lenpos == 2:  a += (data[p+0] + (data[p+1]<<8))
	if lenpos == 1:  a += data[p+0]
	a &= 0xffffffff; b &= 0xffffffff; c &= 0xffffffff
	if lenpos == 0: return c, b

	a, b, c = final(a, b, c)

	return c, b

def hashlittle(data, initval=0):
	c, b = hashlittle2(data, initval, 0)
	return c
