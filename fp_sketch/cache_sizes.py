import math

N = 1 # direct mapped

cache_num_blocks = 4096
blockSize_words = 4
addressSize_bits = 32

# number of sets determined by associativity; N=1 is direct
def cache_num_sets():
    return cache_num_blocks / N

# total number of tag bits in the cache
def tag_bits():
    a = math.log(cache_num_blocks, 2) # 12
    b = math.log(cache_num_sets(), 2) # 12 -> 11 -> 10 (based on N)
    
    base = addressSize_bits - blockSize_words

    s = base - b
    
    c = s * cache_num_blocks
    
    return c


# run test suite
def tests():
    print("num sets:\t%s" % cache_num_sets())
    print("tag bits:\t%s" % tag_bits())
    print()


# test 1
N = 1
print("DIRECT-MAPPED; N=1")
tests()

# test 2
N = 2
print("SET-ASSOCIATIVE; N=2")
tests()

# test 3
N = 4
print("SET-ASSOCIATIVE; N=4")
tests()

# test 4
N = cache_num_blocks
print("FULLY-ASSOCIATIVE; N=%s" % N)
tests()

