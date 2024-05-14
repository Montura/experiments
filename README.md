## BTree-based C++ library for Key-Value pairs storage 

The repository contains the C++17 template-based header library for Windows/Linux/MacOs platforms for storing KEY|VALUES pairs on disk.

### Terms
  * K is a key type 
  * V is a value type 
  * { K key, V value }
  * Volume<K, V>
  * StorageBase<K, V>
  * VolumeMT<K, V>
  * StorageMT<K, V>

### Volume<K, V>
  * is used to answer the queries:
    * `bool exist(K key);` 
    * `void set(K key, V value);`
    * `void set(K key, V value, int size);`
    * `V get(K key);` 
    * `void get(K key);` 
  * is managed by `Storage<K, V>` _object_:
    * `Storage<K, V>` _object_ defines the types of `Volume<K, V>`
  * contains:
    * hierarchical data structure (`Btree`) -> to pass the queries to it
    * `IOManager` object -> to use in `BTree` to perform IO operations
  * the results of _modifing_ queries are written to a file on disk
  * the results of _non-modifing_ queries are read from the file
  * file layout:      
      * <details>
          <summary>header layout (13 bytes)</summary>

              - T                        |=> takes 2 bytes (tree degree)
              - KEY_SIZE                 |=> takes 1 byte
              - VALUE_TYPE               |=> takes 1 byte 
                 - VALUE_TYPE = 0 for integer primitives: int32_t, int64_t
                 - VALUE_TYPE = 1 for unsigned integer primitives: uint32_t, uint64_t
                 - VALUE_TYPE = 2 for floating-point primitives: float, double
                 - VALUE_TYPE = 3 for container of values: (w)string
                 - VALUE_TYPE = 4 for blob                
              - ELEMENT_SIZE             |=> takes 1 byte 
                 - ELEMENT_SIZE = sizeof(VALUE_TYPE) for primitives
                 - ELEMENT_SIZE = sizeof(VALUE_SUBTYPE) for containers or blob

              - ROOT POS                 |=> takes 8 bytes (pos in file)
         </details>
      * <details>
          <summary>node layout</summary>
   
              - FLAG                     |=> takes 1 byte                 (for "is_leaf")
              - USED_KEYS                |=> takes 2 bytes                (for the number of "active" keys in the node)
              - KEY_POS                  |=> takes (2 * t - 1) * KEY_SIZE (for key positions in file)
              - CHILD_POS                |=> takes (2 * t) * KEY_SIZE     (for key positions in file)
        </details>
      * <details>
          <summary>entry layout</summary>
         
                 - KEY                   |=> takes KEY_SIZE bytes (4 bytes is enough for 10^8 different keys)
              ----------–-----
                 - VALUE                 |=> takes ELEMENT_SIZE bytes for primitive VALUE_TYPE
              or
                 - VALUE_SIZE            |=> takes 4 bytes
                 - VALUE                 |=> takes (ELEMENT_SIZE * NUMBER_OF_ELEMENTS) bytes for (w)string or blob VALUE_TYPE
              ----------–-----
        </details>

### Storage <K, V>
  * *storage* template args `<K, V>` define the types of `{ key, value }`
    * ```Storage<int, int> s;```
  * is used to manage `Volume<K,V>` _objects_ through a `std::unique_ptr`:
    * _storage_ owns _volume objects_ and they can't be opened by another _storage_
    * _volume objects_ are disposed automatically when the _storage_ lifetime expires 
  * interface:
     * `VolumeWrapper open_volume(string path, int tree_order);`
     * `void close_volume(VolumeWrapper v);`
     * `VolumeWrapper`:
       * an _object_ with a non-owning raw poiner to the `Volume<K,V>`
  * contains:
    * map of `Volume<K,V>` _objects_

### VolumeMT<K, V>
  * is used to answer to queries in multithreading environment
  * is managed by `StorageMT<K, V>` _object_
  * thread safety is guaranteed with *coarse-grained synchronization*
  * contains:
    * `Volume<K V>` _object_
    * `mutex` _object_ -> is used for synchronization

### StorageMT <K, V>
  * is a `Storage <K, V>` for managing `VolumeMT<K, V>` _objects_
  * interface:
     * `VolumeWrapper open_volume(string path, int tree_order);`
     * `void close_volume(VolumeWrapper v);`
     * `VolumeWrapper`:
       * an _object_ with a non-owning raw poiner to the `VolumeMT<K,V>`
  * contains:
    * map of `VolumeMT<K,V>` _objects_


### Build

#### Requirements
   - [Boost Library](https://www.boost.org/users/history/version_1_71_0.html):  min v. 1.71.0
   - [Boost Iostreams Library](https://www.boost.org/doc/libs/1_76_0/libs/iostreams/doc/index.html)
   - [Boost Thread Library](https://www.boost.org/doc/libs/1_78_0/doc/html/thread.html)
```
$ git clone https://github.com/Montura/experiments.git
$ cd experiments
```

* <details> 
   <summary>Unix systems</summary>
   
   ```
   $ cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   $ cmake --build build --target key-value-storage-test
   $ ./build/test/key-value-storage-test --log_level=success
   ```
</details>
   
*  <details> 
      <summary>Windows x64|x86</summary> 
      
      * x64
         ```
         $ cmake -B build_x64 -S . -DCMAKE_BUILD_TYPE=Release -A x64
         $ cmake --build build_x64 --target key-value-storage-test
         $ %vs2019_install%\MSBuild\Current\Bin\MSBuild.exe build_x64\experiments.sln /p:Configuration=Release
         $ build_64\test\Release\key-value-storage-test.exe --log_level=success
         ```
   
      *  x86   
         ```
         $ cmake -B build_x86 -S . -DCMAKE_BUILD_TYPE=Release -A win32
         $ cmake --build build_x86 --target key-value-storage-test
         $ %vs2019_install%\MSBuild\Current\Bin\MSBuild.exe build_x86\experiments.sln /p:Configuration=Release
         $ build_x86\test\Release\key-value-storage-test.exe --log_level=success
         ```
   </details>

### Verified
* tested on value types:
    *  `int32_t`, `int64_t`, `float`, `double`
    *  `std::string`, `std::wstring`
    *  `blob {const char*, size}`
* tested on 
    * MacOS (x86-64), compiler Apple clang version 13.0.0 (clang-1300.0.29.3, Mac OS Big Sur v11.5.2)
    * Windows (x86|x86-64), Visual Studio 2019 Version 16.5.0 (cl v19.25.28610.4, Windows 10 Pro)
    * Linux (x86-64), compiler GNU version 10.3.0 (Ubuntu v20.04)

### Usage examples
   * <details> 
       <summary>basic usage example</summary> 

       * [void usage()](test/test.cpp#L70)
   
       ```cpp
       {
         btree::Storage<int, int> int_storage;
         auto volume = int_storage.open_volume("../int_storage.txt", 2);
         int val = -1;
         volume.set(0, val);
         std::optional<int> opt = volume.get(0);
         assert(opt.value() == val);
       }
       {
         btree::Storage<int, std::string> str_storage;
         auto volume = str_storage.open_volume("../str_storage.txt", 2);
         std::string val = "abacaba";
         volume.set(0, val);
         std::optional<std::string> opt = volume.get(0);
         assert(opt.value() == val);
       }
       {
         btree::Storage<int, const char*> blob_storage;
         auto volume = blob_storage.open_volume("../blob_storage.txt", 2);
         int len = 10;
         auto blob = std::make_unique<char*>(len + 1);
         for (int i = 0; i < len; ++i) {
             blob[i] = (char)(i + 1);
         }
         volume.set(0, blob.get(), len);
  
         std::optional<const char*> opt = volume.get(0);
         auto ptr = opt.value();
         for (int i = 0; i < len; ++i) {
             assert(ptr[i] == (*blob)[i]);
         }
       }
       ```
     </details>

   * <details> 
      <summary>multithreading usage</summary> 

      * [mt_usage()](test/test.cpp#L125)
   
      ```cpp
      btree::StorageMT<int, int> int_storage;
      auto volume = int_storage.open_volume("../mt_int_storage.txt", 100);

      int n = 100000;
      {
        ThreadPool tp { 10 };
        auto ranges = generate_ranges(n, 10); // ten not-overlapped intervals
        for (auto& range: ranges) {
            tp.post([&volume, &range]() {
                for (int i = range.first; i < range.second; ++i)
                    volume.set(i, -i);
            });
        }
        tp.join();
        for (int i = 0; i < n; ++i)
            assert(volume.get(i).value() == -i);
      }
      {
        ThreadPool tp { 10 };
        tp.post([&volume, n]() {
            for (int i = 0; i < n; ++i)
                volume.set(i, 0);
        });
        tp.join();
        for (int i = 0; i < n; ++i)
            assert(volume.get(i).value() == 0);
      }
      ```
     </details>
   
### Known problems
   * exceeding the limit of the available VirtualAddress space on `x86` in [stress test.h](test/stress_test.h):
      * `boost` can't allocate `mapped_region` for 800mb+ file
      * `possible solution`:  map a fixed-size file part instead of the whole file 
   * resizing of the file on Windows causes the same error as described in the issue: [dotCover crashing - Can't set eof error](https://youtrack.jetbrains.com/issue/PROF-752)
      * ``` [WIN32 error] = 1224, The requested operation cannot be performed on a file with a user-mapped section open.```
      * my case: 
         * caused by [`SetEndOfFile`](https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setendoffile):
           * ``` CreateFileMapping is called to create a file mapping object for hFile, UnmapViewOfFile must be called first to unmap all views and call CloseHandle to close the file mapping object before you can call SetEndOfFile.```
         * `solution`: to destory `boost::interprocess::mapped_region` object before the `std::filesystem::resilze_file(path)` call
   * crash or leaks during the *explicit memory check* performed by overriding global `new` and `delete` in [mem_util.h](test/utils/mem_util.h):
      1. when use `unordered_map` to store allocated chuncks:
         * crashes with the linked `boost libs` on `delete` (seen in debugger)
         * works fine without `boost` (used for testing algo [in branch without IO operations](https://github.com/Montura/experiments/tree/mmap))
      2. when use `uint32_t array[n]` of store allocated chuncks:
         * works without crashes with `boost libs` but reports about leaks (from boost I suppose)

### TODO
<details>
    <summary>todo-list</summary>
   
   * automatic removal of the expiring keys (see [Redis impl](https://github.com/redis/redis/blob/a92921da135e38eedd89138e15fe9fd1ffdd9b48/src/expire.c#L98))
   * a technique for providing atomicity and durability [Write-Ahead-Log](https://people.eecs.berkeley.edu/~kubitron/cs262/handouts/papers/a1-graefe.pdf)   
      * the recovery log describes changes before any in-place updates of the `B-tree`
      * for now all modifications are written at the end of the same file -> file size accordingly grows (drawback)
   * to specify mapped region usage [behavior](https://github.com/steinwurf/boost/blob/master/boost/interprocess/mapped_region.hpp#L199) to reduce [overhead in memory mapped file I/O](https://www.usenix.org/sites/default/files/conference/protected-files/hotstorage17_slides_choi.pdf)
</details>

#### Links:
   * <details> 
      <summary>Overview of data structures for Key|Value storage</summary>

      * [МФТИ. Липовский Р.Г. Теория отказоустойчивых распределенных систем](https://mipt.ru/online/algoritmov-i-tekhnologiy/teoriya-ORS.php)
      * TFTDS 0. Модель распределенной системы
         * [примеры систем](https://youtu.be/HJaI4lCgPCs?t=1106)
      * TFTDS 1. Линеаризуемость. Репликация атомарного регистра, алгоритм ABD
         *  [k/v storage](https://youtu.be/FWQ37wvq1OI?t=619)
         *  [реализация k/v storage](https://youtu.be/FWQ37wvq1OI?t=2441)
      * TFTDS. Семинар 2. Локальное хранилище
         *  [B-tree](https://youtu.be/wXoQIh6mvwE?t=2806)
         *  [LSM-tree](https://youtu.be/wXoQIh6mvwE?t=3447)
      *  [Блеск и нищета key-value базы данных LMDB в приложениях для iOS](https://habr.com/ru/company/vk/blog/480850/)
      *  [Understanding Key-Value Store’s Internal Working](https://medium.com/swlh/key-value-pair-database-internals-18f52c36bb70)
      *  [The State of the Storage Engine](https://dzone.com/articles/state-storage-engine)
      *  [B-Tree vs Log-Structured-Merge-Tree](https://tikv.github.io/deep-dive-tikv/key-value-engine/B-Tree-vs-Log-Structured-Merge-Tree.html)
      *  [Btree vs LSM (WiredTiger bench)](https://github.com/wiredtiger/wiredtiger/wiki/Btree-vs-LSM)
      *  [Closing the B-tree vs. LSM-tree Write Amplification Gap on Modern Storage Hardware with Built-in Transparent Compression (WiredTiger article)](https://arxiv.org/pdf/2107.13987.pdf)
   </details>
   
   *  <details> 
         <summary>Mapped files</summary> 
   
         * [Introduction to Memory Mapped IO]( https://towardsdatascience.com/introduction-to-memory-mapped-io-3540454770f7)
         * [Efficient Memory Mapped File I/O for In-Memory File Systems](https://www.usenix.org/sites/default/files/conference/protected-files/hotstorage17_slides_choi.pdf)
      </details>
   
   * <details> 
      <summary>Overview of BTree impls</summary> 
   
      *  [B-tree library for eventual proposal to Boost](https://github.com/Beman/btree)
      *  [B-tree based on Google's B-tree implementation](https://github.com/Kronuz/cpp-btree)
      *  [Fine-Grained-Locked-B-Tree](https://github.com/MentallyCramped/Fine-Grained-Locked-B-Tree)
      *  [BPlusTree](https://github.com/skyzh/BPlusTree)
      *  [B-tree ИМТО конспект](https://neerc.ifmo.ru/wiki/index.php?title=B-%D0%B4%D0%B5%D1%80%D0%B5%D0%B2%D0%BE)
      *  [Implement Key-Value Store by B-Tree on Linux OS environment](https://medium.com/@pthtantai97/implement-key-value-store-by-btree-5a100a03da3a)
         * [B-Tree impl on Linux OS environment](https://github.com/phamtai97/key-value-store)
         * [Key Value Store using B-Tree](https://github.com/billhcmus/key-value-store)
     </details>
