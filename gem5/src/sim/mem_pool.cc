/*
 * Copyright (c) 2011-2021 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sim/mem_pool.hh"

#include "base/addr_range.hh"
#include "base/logging.hh"

#include <cstdlib> 
#include <ctime> 
#include <random>

namespace gem5
{
int tp;
MemPool::MemPool(Addr page_shift, Addr ptr, Addr limit)
        : pageShift(page_shift), startPageNum(ptr >> page_shift),
        freePageNum(ptr >> page_shift),
        _totalPages((limit - ptr) >> page_shift)
      
{
        gem5_assert(_totalPages > 0);
    tp=_totalPages;
    srand((unsigned int) time(NULL));
}

Counter
MemPool::startPage() const
{
    return startPageNum;
}

Counter
MemPool::freePage() const
{
    return freePageNum;
}

void
MemPool::setFreePage(Counter value)
{
    freePageNum = value;
}

Addr
MemPool::freePageAddr() const
{
    return freePageNum << pageShift;
}

Counter
MemPool::totalPages() const
{
    return _totalPages;
}

Counter
MemPool::allocatedPages() const
{
    return freePageNum - startPageNum;
}

Counter
MemPool::freePages() const
{
    return _totalPages - allocatedPages();
}

Addr
MemPool::startAddr() const
{
    return startPage() << pageShift;
}

Addr
MemPool::allocatedBytes() const
{
    return allocatedPages() << pageShift;
}

Addr
MemPool::freeBytes() const
{
    return freePages() << pageShift;
}

Addr
MemPool::totalBytes() const
{
    return totalPages() << pageShift;
}
std::vector<int> freelist;
std::vector<bool> bitmap;

int block_size = 0;
int currentnum=0;

void shift_random(std::vector<int> &random_numbers){
        std::vector<int> random_numbers_2= random_numbers;
        for (int j = 0; j < random_numbers.size()-1; j++) {
            random_numbers[j]=random_numbers[j+1] ;
        }
        random_numbers[random_numbers.size()-1]=random_numbers_2[0];
}

std::vector<int> make_random(int num){

        std::vector<int> randArr;
        std::vector<int> arr (num,0);
        for(int i=0;i<num;i++){
            arr[i]=i;
        }
        std::random_device rnd;
        std::mt19937 mt(rnd());
        srand(time(nullptr)); 
        for (int i = 0, len = arr.size(); i < num; i++, len--) {
            int randNum = mt() % len; 
            randArr.push_back(arr[randNum]);
            arr.erase(arr.begin() + randNum);
        }
        return randArr;
}

void require_page(std::vector<int> &random_numbers){
    int tmpl=0,next=0;
    for(int j=0;j<block_size;j++){
        next=random_numbers[j]+tmpl;
        tmpl+=block_size;
        freelist.push_back(next);
    }
    shift_random(random_numbers);
}


bool check=false;
Addr
MemPool::allocate(Addr npages, std::vector<int> &random_numbers)
{
    if(check==false){
        bitmap.resize(tp,false);
        block_size = sqrt(tp);
        random_numbers = make_random(block_size);
        for(int i=0;i<random_numbers.size();i++){
            std::cout<<random_numbers[i]<<" "<<std::endl;
        }
        require_page();
        check=true;
    }
    fatal_if(freePages() <= 0,
            "Out of memory, please increase size of physical memory.");
    
    if(freelist.size()<=currentnum){
        require_page();
    }
    unsigned long pageNum = freelist[currentnum];
    Addr allocatedAddr = (pageNum << pageShift);
    currentnum++;

    return allocatedAddr;
}

std::vector<std::vector<int>> numberslist;
std::vector<int> tmp(random_numbers.size());

void make_numberslist(){
  for (int shift = 0; shift < random_numbers.size(); ++shift) {
    for (int i = 0; i < random_numbers.size(); ++i) {
        tmp[i] = random_numbers[i] + i * random_numbers.size();
    }
    numberslist.push_back(tmp);
    std::rotate(random_numbers.begin(), random_numbers.begin() + 1, random_numbers.end());
  }
}


void MemPool::deallocate(Addr start, Addr npages) {
    assert(npages == Addr(1));
    assert(((start >> pageShift) << pageShift) == start);
    if(numberslist.size()==0) make_numberslist();

    Addr pageNumber = start >> pageShift;
    if (bitmap[pageNumber]) {
        bitmap[pageNumber] = false;

        std::vector<int> tempFreelist = freelist;
        tempFreelist.push_back(pageNumber);
        std::sort(tempFreelist.begin(), tempFreelist.end());

        bool exclude = false;
        std::vector<int> matched_sequence;

        for (const auto& seq : numberslist) {
            std::vector<int> sorted_seq = seq;
            std::sort(sorted_seq.begin(), sorted_seq.end());
            if (std::includes(tempFreelist.begin(), tempFreelist.end(), sorted_seq.begin(), sorted_seq.end())) {
                exclude = true;
                matched_sequence = seq;
                break;
            }
        }

        if (exclude) {
            for (int idx : matched_sequence) {
                if (idx >= 0 && idx < bitmap.size()) {
                    bitmap[idx] = false;
                }
            }
            return;
        }
        freelist.push_back(pageNumber);
        currentnum--;

    } else {
        fatal("Double free detected on page: %d\n", pageNumber);
    }
}
void
MemPool::serialize(CheckpointOut &cp) const
{
    paramOut(cp, "page_shift", pageShift);
    paramOut(cp, "start_page", startPageNum);
    paramOut(cp, "free_page_num", freePageNum);
    paramOut(cp, "total_pages", _totalPages);
}

void
MemPool::unserialize(CheckpointIn &cp)
{
    paramIn(cp, "page_shift", pageShift);
    paramIn(cp, "start_page", startPageNum);
    paramIn(cp, "free_page_num", freePageNum);
    paramIn(cp, "total_pages", _totalPages);
}

void
MemPools::populate(const AddrRangeList &memories)
{
    for (const auto &mem : memories)
        pools.emplace_back(pageShift, mem.start(), mem.end());
}

Addr
MemPools::allocPhysPages(int npages, int pool_id)
{
    return pools[pool_id].allocate(npages);
}

void
MemPools::deallocPhysPages(Addr start, int npages, int pool_id)
{
    pools[pool_id].deallocate(start, npages);
}
  

Addr
MemPools::memSize(int pool_id) const
{
    return pools[pool_id].totalBytes();
}

Addr
MemPools::freeMemSize(int pool_id) const
{
    return pools[pool_id].freeBytes();
}

void
MemPools::serialize(CheckpointOut &cp) const
{
    ScopedCheckpointSection sec(cp, "mempools");
    int num_pools = pools.size();
    SERIALIZE_SCALAR(num_pools);

    for (int i = 0; i < num_pools; i++)
        pools[i].serializeSection(cp, csprintf("pool%d", i));
}

void
MemPools::unserialize(CheckpointIn &cp)
{
    // Delete previous mem_pools
    pools.clear();

    ScopedCheckpointSection sec(cp, "mempools");
    int num_pools = 0;
    UNSERIALIZE_SCALAR(num_pools);

    for (int i = 0; i < num_pools; i++) {
        MemPool pool;
        pool.unserializeSection(cp, csprintf("pool%d", i));
        pools.push_back(pool);
    }
}

} // namespace gem5