import math

# lil helper function
def MiB(n_bytes):
    return n_bytes / math.pow(2,20)

def bitsToBytes(n_bits):
    return n_bits / 8


# constants
v_addr_bits = 32
p_addr_bits = 32
mempage_bytes = 4096


# function definitions
def numPages(v_addr_bits, page_bytes):
    # calculate num. bits required to make a virtual address
    page_num_bits = math.ceil(math.log(page_bytes, 2))

    # calculate power (to be of 2) to address all pages
    power = v_addr_bits - page_num_bits

    # return 2^power
    return int(math.pow(2, power))


def bitsPerEntry(p_addr_bits, page_bytes):
    status_bits = 1 + 1 + 1
    # PTE width is physical address width + status(es) width
    return p_addr_bits + status_bits


def PTSize(v_addr_bits, p_addr_bits, mempage_bytes):
    np = numPages(v_addr_bits, mempage_bytes)
    bpe = bitsPerEntry(p_addr_bits, mempage_bytes)
    return np * bpe

# IT'S GOOOD!!!

# tests
print("%s pages" % numPages(v_addr_bits, mempage_bytes))
print("%s bits per entry" % bitsPerEntry(p_addr_bits, mempage_bytes))

s = PTSize(v_addr_bits, p_addr_bits, mempage_bytes)
print("Page table requires %s bits to implement." % s)
print("That's %s MiB!" % MiB(bitsToBytes(s)))
