#include "GenericEXT4.h"

/*
 * We can use this to read all Group Descs
 * at the Mount func to easily be able to
 * use them later on.
 * 
 * To read a Group Descriptor we need to first 
 * get the block Size and the sectors per
 * block. Then we can get the First GDT Block
 * and LBA and the desc Size, Offset, LBA and
 * Byte Offset. Then we can read the block 
 * group descriptor.
*/
BlockGroupDescriptor* GenericEXT4Device::ReadGroupDesc(uint32_t group) {
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    uint64_t FirstGDTBlock = (blockSize == 1024) ? 2 : 1;
    uint64_t FirstGDTLBA = FirstGDTBlock * sectorsPerBlock;

    uint64_t descSize = superblock->GroupDescriptorBytes ? superblock->GroupDescriptorBytes : 64;
    uint64_t descByteOffset = group * descSize;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t descLBA = FirstGDTLBA + (descByteOffset / sectorSize);
    uint64_t descOff = descByteOffset % sectorSize;

    uint64_t bytesNeeded = descOff + descSize;
    uint64_t sectorsNeeded = (bytesNeeded + sectorSize - 1) / sectorSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectorsNeeded; i++) {
        if (!pdev->ReadSector(descLBA + i, (void*)((uint8_t*)bufPhys + i * sectorSize))) {
            _ds->Print("GDT read failed");
        }
    }
    return (BlockGroupDescriptor*)((uint8_t*)bufVirt + descOff);
}

/*
 * To write to an update GroupDesc 
 * we need to first update the Block
 * Bitmap and the Inode Bitmap. Then
 * we can crc32 the GroupDesc.
*/
void GenericEXT4Device::UpdateGroupDesc(uint32_t group, BlockGroupDescriptor* GroupDesc) {
    if (superblock->MetaCheckAlgo == 1) {
        GroupDesc->LowChkInodeBitmap = 0;
        GroupDesc->HighChkInodeBitmap = 0;
        GroupDesc->LowChkBlockBitmap = 0;
        GroupDesc->HighChkBlockBitmap = 0;

        UpdateBlockBitmapChksum(group, GroupDesc);
        UpdateInodeBitmapChksum(group, GroupDesc);
        
        GroupDesc->Checksum = 0;

        uint32_t crc = crc32c_sw(superblock->CheckUUID, &group, sizeof(group));
        crc = crc32c_sw(crc, (uint8_t*)GroupDesc, superblock->GroupDescriptorBytes);

        GroupDesc->Checksum = crc & 0xFFFF;
    }
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / sectorSize;

    uint64_t firstGDTBlock = (blockSize == 1024) ? 2 : 1;
    uint64_t firstGDTLBA = firstGDTBlock * sectorsPerBlock;

    uint64_t descSize = superblock->GroupDescriptorBytes ? superblock->GroupDescriptorBytes : 64;

    uint64_t descByteOffset = (uint64_t)group * descSize;
    uint64_t descLBA = firstGDTLBA + (descByteOffset / sectorSize);
    uint64_t descOff = descByteOffset % sectorSize;

    uint64_t bytes = descOff + descSize;
    uint64_t sectors = (bytes + sectorSize - 1) / sectorSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectors; i++) {
        if (!pdev->ReadSector(descLBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("GDT read failed");
            return;
        }
    }

    memcpy((uint8_t*)bufVirt + descOff, GroupDesc, descSize);

    for (uint64_t i = 0; i < sectors; i++) {
        if (!pdev->WriteSector(descLBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("GDT write failed");
            return;
        }
    }
}