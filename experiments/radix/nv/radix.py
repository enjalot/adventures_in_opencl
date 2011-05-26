#http://documen.tician.de/pyopencl/

import pyopencl as cl
import numpy as np
import struct

import timing
timings = timing.Timing()

#ctx = cl.create_some_context()
mf = cl.mem_flags

class Radix:
    def __init__(self, max_elements, cta_size, dtype):
        self.WARP_SIZE = 32
        self.SCAN_WG_SIZE = 256
        self.MIN_LARGE_ARRAY_SIZE = 4 * self.SCAN_WG_SIZE
        self.bit_step = 4
        self.cta_size = cta_size
        self.uintsz = dtype.itemsize

        plat = cl.get_platforms()[0]
        device = plat.get_devices()[0]
        self.ctx = cl.Context(devices=[device])
        self.queue = cl.CommandQueue(self.ctx, device)

        self.loadProgram()

        if (max_elements % (cta_size * 4)) == 0:
            num_blocks = max_elements / (cta_size * 4)
        else:
            num_blocks = max_elements / (cta_size * 4) + 1

        #print "num_blocks: ", num_blocks
        self.d_tempKeys = cl.Buffer(self.ctx, mf.READ_WRITE, size=self.uintsz * max_elements)
        self.d_tempValues = cl.Buffer(self.ctx, mf.READ_WRITE, size=self.uintsz * max_elements)

        self.mCounters = cl.Buffer(self.ctx, mf.READ_WRITE, size=self.uintsz * self.WARP_SIZE * num_blocks)
        self.mCountersSum = cl.Buffer(self.ctx, mf.READ_WRITE, size=self.uintsz * self.WARP_SIZE * num_blocks)
        self.mBlockOffsets = cl.Buffer(self.ctx, mf.READ_WRITE, size=self.uintsz * self.WARP_SIZE * num_blocks)

        numscan = max_elements/2/cta_size*16
        #print "numscan", numscan
        if numscan >= self.MIN_LARGE_ARRAY_SIZE:
        #MAX_WORKGROUP_INCLUSIVE_SCAN_SIZE 1024
            self.scan_buffer = cl.Buffer(self.ctx, mf.READ_WRITE, size = self.uintsz * numscan / 1024)



    def loadProgram(self):
        print "build scan"
        f = open("Scan_b.cl", 'r')
        fstr = "".join(f.readlines())
        self.scan_prg = cl.Program(self.ctx, fstr).build()
        print "build radix"
        f = open("RadixSort.cl", 'r')
        fstr = "".join(f.readlines())
        self.radix_prg = cl.Program(self.ctx, fstr).build()


    @timings("Radix Sort")
    def sort(self, num, keys_np, values_np):
        self.keys = cl.Buffer(self.ctx, mf.READ_WRITE | mf.COPY_HOST_PTR, hostbuf=keys_np)
        self.values = cl.Buffer(self.ctx, mf.READ_WRITE | mf.COPY_HOST_PTR, hostbuf=values_np)
        key_bits = keys_np.dtype.itemsize * 8
        #print "numElements", num
        #print "key_bits", key_bits
        #print "bit_step", self.bit_step
        
        i = 0
        while key_bits > i*self.bit_step:
            #print "i*bit_step", i*self.bit_step
            self.step(self.bit_step, i*self.bit_step, num);
            i += 1;

        self.queue.finish()
        cl.enqueue_read_buffer(self.queue, self.keys, keys_np).wait()
        cl.enqueue_read_buffer(self.queue, self.values, values_np).wait()
        return keys_np, values_np


    @timings("Radix: step")
    def step(self, nbits, startbit, num):
        self.blocks(nbits, startbit, num)
        self.queue.finish()

        self.find_offsets(startbit, num)
        self.queue.finish()

        array_length = num/2/self.cta_size*16
        #print "array length in step", array_length
        if array_length < self.MIN_LARGE_ARRAY_SIZE:
            self.naive_scan(num)
        else:
            self.scan(self.mCountersSum, self.mCounters, 1, array_length);
        self.queue.finish()
        #self.naive_scan(num)

        self.reorder(startbit, num)
        self.queue.finish()


    @timings("Radix: blocks")
    def blocks(self, nbits, startbit, num):
        totalBlocks = num/4/self.cta_size
        global_size = (self.cta_size*totalBlocks,)
        local_size = (self.cta_size,)
        blocks_args = ( self.keys,
                        self.values,
                        self.d_tempKeys,
                        self.d_tempValues,
                        np.uint32(nbits),
                        np.uint32(startbit),
                        np.uint32(num),
                        np.uint32(totalBlocks),
                        cl.LocalMemory(4*self.cta_size*self.uintsz),
                        cl.LocalMemory(4*self.cta_size*self.uintsz)
                    )
        self.radix_prg.radixSortBlocksKeysValues(self.queue, global_size, local_size, *(blocks_args)).wait()
        #self.radix_prg.radixSortBlocksKeysOnly(self.queue, global_size, local_size, *(blocks_args)).wait()


    @timings("Radix: find offsets")
    def find_offsets(self, startbit, num):
        totalBlocks = num/2/self.cta_size
        global_size = (self.cta_size*totalBlocks,)
        local_size = (self.cta_size,)
        offsets_args = ( self.d_tempKeys,
                         self.d_tempValues,
                         self.mCounters,
                         self.mBlockOffsets,
                         np.uint32(startbit),
                         np.uint32(num),
                         np.uint32(totalBlocks),
                         cl.LocalMemory(2*self.cta_size*self.uintsz),
                    )
        self.radix_prg.findRadixOffsets(self.queue, global_size, local_size, *(offsets_args)).wait()


    @timings("Radix: naive scan")
    def naive_scan(self, num):
        nhist = num/2/self.cta_size*16
        global_size = (nhist,)
        local_size = (nhist,)
        extra_space = nhist / 16 #NUM_BANKS defined as 16 in RadixSort.cpp
        shared_mem_size = self.uintsz * (nhist + extra_space)
        scan_args = (   self.mCountersSum,
                        self.mCounters,
                        np.uint32(nhist),
                        cl.LocalMemory(2*shared_mem_size)
                    )
        self.radix_prg.scanNaive(self.queue, global_size, local_size, *(scan_args)).wait()


    @timings("Radix: scan")
    def scan(self, dst, src, batch_size, array_length):
        self.scan_local1(   dst, 
                            src, 
                            batch_size * array_length / (4 * self.SCAN_WG_SIZE), 
                            4 * self.SCAN_WG_SIZE)
        
        self.queue.finish()
        self.scan_local2(   dst, 
                            src, 
                            batch_size,
                            array_length / (4 * self.SCAN_WG_SIZE))
        self.queue.finish()
        self.scan_update(dst, batch_size * array_length / (4 * self.SCAN_WG_SIZE))
        self.queue.finish()

    
    @timings("Scan: local1")
    def scan_local1(self, dst, src, n, size):
        global_size = (n * size / 4,)
        local_size = (self.SCAN_WG_SIZE,)
        scan_args = (   dst,
                        src,
                        cl.LocalMemory(2 * self.SCAN_WG_SIZE * self.uintsz),
                        np.uint32(size)
                    )
        self.scan_prg.scanExclusiveLocal1(self.queue, global_size, local_size, *(scan_args)).wait()

    @timings("Scan: local2")
    def scan_local2(self, dst, src, n, size):
        elements = n * size
        dividend = elements
        divisor = self.SCAN_WG_SIZE
        if dividend % divisor == 0:
            global_size = (dividend,)
        else: 
            global_size = (dividend - dividend % divisor + divisor,)

        local_size = (self.SCAN_WG_SIZE, )
        scan_args = (   self.scan_buffer,
                        dst,
                        src,
                        cl.LocalMemory(2 * self.SCAN_WG_SIZE * self.uintsz),
                        np.uint32(elements),
                        np.uint32(size)
                    )
        self.scan_prg.scanExclusiveLocal2(self.queue, global_size, local_size, *(scan_args)).wait()


    @timings("Scan: update")
    def scan_update(self, dst, n):
        global_size = (n * self.SCAN_WG_SIZE,)
        local_size = (self.SCAN_WG_SIZE,)
        scan_args = (   dst,
                        self.scan_buffer
                    )
        self.scan_prg.uniformUpdate(self.queue, global_size, local_size, *(scan_args)).wait()

    @timings("Scan: reorder")
    def reorder(self, startbit, num):
        totalBlocks = num/2/self.cta_size
        global_size = (self.cta_size*totalBlocks,)
        local_size = (self.cta_size,)
        reorder_args = ( self.keys,
                         self.values,
                         self.d_tempKeys,
                         self.d_tempValues,
                         self.mBlockOffsets,
                         self.mCountersSum,
                         self.mCounters,
                         np.uint32(startbit),
                         np.uint32(num),
                         np.uint32(totalBlocks),
                         cl.LocalMemory(2*self.cta_size*self.uintsz),
                         cl.LocalMemory(2*self.cta_size*self.uintsz)
                    )
        self.radix_prg.reorderDataKeysValues(self.queue, global_size, local_size, *(reorder_args))
        #self.radix_prg.reorderDataKeysOnly(self.queue, global_size, local_size, *(reorder_args))


if __name__ == "__main__":

    n = 1048576*2
    #n = 32768*2
    #n = 16384
    #n = 8192
    hashes = np.ndarray((n,1), dtype=np.uint32)
    indices = np.ndarray((n,1), dtype=np.uint32)
    
    for i in xrange(0,n): 
        hashes[i] = n - i
        indices[i] = i
    
    npsorted = np.sort(hashes,0)

    print "hashes before:", hashes[0:20].T
    print "indices before: ", indices[0:20].T 

    radix = Radix(n, 128, hashes.dtype)
    #num_to_sort = 32768
    num_to_sort = n
    hashes, indices = radix.sort(num_to_sort, hashes, indices)

    print "hashes after:", hashes[0:20].T
    print "indices after: ", indices[0:20].T 

    print np.linalg.norm(hashes - npsorted)

    print timings






