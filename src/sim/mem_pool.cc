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

void shift_random(std::vector<int> &tmp){
        std::vector<int> tmp2= tmp;
        for (int j = 0; j < tmp.size()-1; j++) {
            tmp[j]=tmp[j+1] ;
        }
        tmp[tmp.size()-1]=tmp2[0];
}

std::vector<int> make_random(int num){

        std::vector<int> randArr;
        std::vector<int> arr (num,0);
        for(int i=0;i<num;i++){
            arr[i]=i;
        }
        std::random_device rnd;
        std::mt19937 mt(rnd());
        srand(time(nullptr)); // ランダムなシードを生成
        for (int i = 0, len = arr.size(); i < num; i++, len--) {
            int randNum = mt() % len; // 0～len-1の範囲の整数からランダムに値を取得
            randArr.push_back(arr[randNum]); // 配列のランダム値に対応するインデックスを得たうえで元々の配列から取り除く
            arr.erase(arr.begin() + randNum);
        }
        return randArr;
}
std::vector<int> ans;
void require_page(){
    int tmpl=0,next=0;
    for(int j=0;j<block_size;j++){
        next=ans[j]+tmpl;
        tmpl+=block_size;
        freelist.push_back(next);
    }
    shift_random(ans);
}


bool check=false;
Addr
MemPool::allocate(Addr npages)
{
    //Addr return_addr = freePageAddr();
    //freePageNum += npages;
    //std::cout<<"サイズは"<<bitmap.capacity()<<"　tpは"<<tp<<std::endl;

    if(check==false){
        bitmap.resize(tp,false);
        block_size = sqrt(tp);
        ans = make_random(block_size);
        std::cout<<"サイズは"<<bitmap.size()<<"  "<<bitmap[256]<<"　tpは"<<tp<<std::endl;
        require_page();
        check=true;
    }
    //std::cout<<"ページ番号:" <<freelist[tmp]<<std::endl;
    fatal_if(freePages() <= 0,
            "Out of memory, please increase size of physical memory.");
    
    if(freelist.size()<=currentnum){//freelistのサイズよりもtmpが大きくなったらrequire_pageでリストにつなぐ
        require_page();
    }
    Addr allocatedAddr = ((unsigned long)freelist[currentnum++]<<pageShift);
    
    std::cout<<freelist[currentnum]<<"ページ番号をallocate"<<"アドレスは"<<std::hex<< allocatedAddr <<std::endl;
    return allocatedAddr;
}

void MemPool::deallocate(Addr start, Addr npages) {
    assert(npages == Addr(1));
    assert(((start >> pageShift) << pageShift) == start);

    // ページ番号を計算 (start を pageShift でシフト)
    Addr pageNumber = start >> pageShift;
    std::cout<<"返却したいページ番号は"<<pageNumber<<"です"<<std::endl;
    // ページが既に解放されていないか確認
    if (bitmap[pageNumber]) {
        bitmap[pageNumber] = false;   // ビットマップでページを解放済みにする
        std::cout<<"returnしたページ番号は"<<pageNumber<<"です"<<std::endl;
        freelist.push_back(pageNumber);  // `freelist` にページを戻す
        currentnum--;  // 解放したのでインデックスを減らす
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

