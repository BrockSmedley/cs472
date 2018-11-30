// Copyright (c) 2018 Brock Smedley. All rights reserved. 
// this is super-cereal boilerplate code
// Use of this source code is governed by a BSD-style license that can be
// found on... the internet :)

#include <string>
#include <math.h>


// conversion functions
float MiB(float n_bytes){
    return n_bytes / pow(2.0, 20.0);
}
float bitsToBytes(int n_bits){
    return n_bits / 8.0;
}

// number of pages possible with page_size & virt_addr_width
int numPages(int v_addr_bits, int page_bytes){
    // calculate num. bits required to make a virtual address
    int page_num_bits = ceil(log2l(page_bytes));

    // calculate power (to be of 2) to address all pages
    int power = v_addr_bits - page_num_bits;

    // return total # of pages
    return pow(2, power);
}

// how many bits each PTE requires
int bitsPerEntry(int p_addr_bits, int page_bytes){
    int status_bits = 1 + 1 + 1;
    return ceil(p_addr_bits) + status_bits;
}

// composite function; numPages * bitsPerEntry
int pageTableSize(int v_addr_bits, int p_addr_bits, int page_bytes){
    return numPages(v_addr_bits, page_bytes) * bitsPerEntry(p_addr_bits, page_bytes);
}

/*  console args:
        virt_addr_width_bits  phys_addr_width_bits  mempage_size_bytes
*/
int main(int argc, const char **argv){
    int v_addr_bits, p_addr_bits, mempage_bytes;

    /* gather & verify console args */
    if (argc != 4){
        printf("Invalid arguments.\n");
        exit(argc);
    }
    v_addr_bits = (int)std::stod(argv[1]);
    p_addr_bits = (int)std::stod(argv[2]);
    mempage_bytes = (int)std::stod(argv[3]);
    printf("Received args:\nv_addr_width\t%d bits\np_addr_width\t%d bits\npage_width\t%d bytes\n\n", v_addr_bits, p_addr_bits, mempage_bytes);

    /* calculate parameters & print them */
    int n_pages = numPages(v_addr_bits, mempage_bytes);
    int bits_per_entry = bitsPerEntry(p_addr_bits, mempage_bytes);
    printf("Calculated parameters:\nnum_pages\t%d\nbits/entry\t%d\n\n", n_pages, bits_per_entry);

    /* calculate size of page table & print it */
    int pts_bits = pageTableSize(v_addr_bits, p_addr_bits, mempage_bytes);
    printf("This page table requires %d bits to implement.\nThat's %f MiB!\n", pts_bits, MiB(bitsToBytes(pts_bits)));

    return 0;
}