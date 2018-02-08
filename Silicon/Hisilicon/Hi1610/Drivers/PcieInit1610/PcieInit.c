/** @file
*
*  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2016, Linaro Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "PcieInit.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/OemMiscLib.h>
#include <Library/PciExpressLib.h>
#include <Library/PlatformPciLib.h>


extern VOID PcieRegWrite(UINT32 Port, UINTN Offset, UINT32 Value);
extern EFI_STATUS PciePortReset(UINT32 HostBridgeNum, UINT32 Port);
extern EFI_STATUS PciePortInit (UINT32 soctype, UINT32 HostBridgeNum, PCIE_DRIVER_CFG *PcieCfg);

PCIE_DRIVER_CFG gastr_pcie_driver_cfg[PCIE_MAX_ROOTBRIDGE] =
{
    //Port 0
    {
        0x0,                        //Portindex

        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        }, //PortInfo

    },

    //Port 1
    {
        0x1,                        //Portindex
        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        },

    },

    //Port 2
    {
        0x2,                        //Portindex
        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        },

    },

    //Port 3
    {
        0x3,                        //Portindex
        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        },

    },
    //Port 4
    {
        0x4,                        //Portindex
        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        },

    },
    //Port 5
    {
        0x5,                        //Portindex
        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        },

    },
    //Port 6
    {
        0x6,                        //Portindex
        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        },

    },
    //Port 7
    {
        0x7,                        //Portindex
        {
            PCIE_ROOT_COMPLEX,      //PortType
            PCIE_WITDH_X8,          //PortWidth
            PCIE_GEN3_0,            //PortGen
        },

    },
};

STATIC
UINT64
GetPcieCfgAddress (
    UINT64 Ecam,
    UINTN Bus,
    UINTN Device,
    UINTN Function,
    UINTN Reg
    )
{
  return Ecam + PCI_EXPRESS_LIB_ADDRESS (Bus, Device, Function, Reg);
}

STATIC
VOID
SetAtuConfig0RW (
    PCI_ROOT_BRIDGE_RESOURCE_APPETURE *Private,
    UINT32 Index
    )
{
  UINTN RbPciBase = Private->RbPciBar;
  UINT64 MemLimit = GetPcieCfgAddress (Private->Ecam, Private->BusBase + 1, 1, 0, 0) - 1;


  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_VIEW_POINT, Index);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LOW, (UINT32)(Private->Ecam));
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_HIGH, (UINT32)((UINT64)(Private->Ecam) >> 32));
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LIMIT, (UINT32) MemLimit);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_LOW, 0);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_HIGH, 0);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL1, IATU_CTRL1_TYPE_CONFIG0);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL2, IATU_SHIIF_MODE);

  {
    UINTN i;
    for (i=0; i<0x20; i+=4) {
      DEBUG ((EFI_D_ERROR, "[%a:%d] - Base=%p value=%x\n", __FUNCTION__, __LINE__, RbPciBase + 0x900 + i, MmioRead32(RbPciBase + 0x900 + i)));
    }
  }
}

void SetAtuConfig1RW (
    PCI_ROOT_BRIDGE_RESOURCE_APPETURE *Private,
    UINT32 Index
    )
{
  UINTN RbPciBase = Private->RbPciBar;
  UINT64 MemLimit = GetPcieCfgAddress (Private->Ecam, Private->BusLimit + 1, 0, 0, 0) - 1;


  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_VIEW_POINT, Index);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL1, IATU_CTRL1_TYPE_CONFIG1);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LOW, (UINT32)(Private->Ecam));
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_HIGH, (UINT32)((UINT64)(Private->Ecam) >> 32));
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LIMIT, (UINT32) MemLimit);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_LOW, 0);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_HIGH, 0);
  MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL2, IATU_SHIIF_MODE);

  {
    UINTN i;
    for (i=0; i<0x20; i+=4) {
      DEBUG ((EFI_D_ERROR, "[%a:%d] - Base=%p value=%x\n", __FUNCTION__, __LINE__, RbPciBase + 0x900 + i, MmioRead32(RbPciBase + 0x900 + i)));
    }
  }
}

void SetAtuIoRW(UINT64 RbPciBase,UINT64 IoBase,UINT64 CpuIoRegionLimit, UINT64 CpuIoRegionBase, UINT32 Index)
{

    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_VIEW_POINT, Index);
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL1, IATU_CTRL1_TYPE_IO);

    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LOW, (UINT32)(CpuIoRegionBase));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_HIGH, (UINT32)((UINT64)CpuIoRegionBase >> 32));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LIMIT, (UINT32)(CpuIoRegionLimit));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_LOW, (UINT32)(IoBase));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_HIGH, (UINT32)((UINT64)(IoBase) >> 32));

    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL2, IATU_NORMAL_MODE);

    {
      UINTN i;
      for (i=0; i<0x20; i+=4) {
        DEBUG ((EFI_D_ERROR, "[%a:%d] - Base=%p value=%x\n", __FUNCTION__, __LINE__, RbPciBase + 0x900 + i, MmioRead32(RbPciBase + 0x900 + i)));
      }
    }
}

void SetAtuMemRW(UINT64 RbPciBase,UINT64 MemBase,UINT64 CpuMemRegionLimit, UINT64 CpuMemRegionBase, UINT32 Index)
{

    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_VIEW_POINT, Index);
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL1, IATU_CTRL1_TYPE_MEM);

    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LOW, (UINT32)(CpuMemRegionBase));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_HIGH, (UINT32)((UINT64)(CpuMemRegionBase) >> 32));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_BASE_LIMIT, (UINT32)(CpuMemRegionLimit));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_LOW, (UINT32)(MemBase));
    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_TARGET_HIGH, (UINT32)((UINT64)(MemBase) >> 32));

    MmioWrite32 (RbPciBase + IATU_OFFSET + IATU_REGION_CTRL2, IATU_NORMAL_MODE);

    {
      UINTN i;
      for (i=0; i<0x20; i+=4) {
        DEBUG ((EFI_D_ERROR, "[%a:%d] - Base=%p value=%x\n", __FUNCTION__, __LINE__, RbPciBase + 0x900 + i, MmioRead32(RbPciBase + 0x900 + i)));
      }
    }
}

VOID InitAtu (PCI_ROOT_BRIDGE_RESOURCE_APPETURE *Private)
{
  SetAtuMemRW (Private->RbPciBar, Private->PciRegionBase, Private->PciRegionLimit, Private->CpuMemRegionBase, 0);
  SetAtuConfig0RW (Private, 1);
  SetAtuConfig1RW (Private, 2);
  SetAtuIoRW (Private->RbPciBar, Private->IoBase, Private->IoLimit, Private->CpuIoRegionBase, 3);
}

EFI_STATUS
PcieInitEntry (
  IN EFI_HANDLE                 ImageHandle,
  IN EFI_SYSTEM_TABLE           *SystemTable
  )

{
    UINT32             Port;
    EFI_STATUS         Status = EFI_SUCCESS;
    UINT32             HostBridgeNum = 0;
    UINT32             soctype = 0;
    UINT32       PcieRootBridgeMask;


    if (!OemIsMpBoot())
    {
        PcieRootBridgeMask = PcdGet32(PcdPcieRootBridgeMask);
    }
    else
    {
        PcieRootBridgeMask = PcdGet32(PcdPcieRootBridgeMask2P);
    }

    soctype = PcdGet32(Pcdsoctype);
    for (HostBridgeNum = 0; HostBridgeNum < PCIE_MAX_HOSTBRIDGE; HostBridgeNum++) {
        for (Port = 0; Port < PCIE_MAX_ROOTBRIDGE; Port++) {
            /*
               Host Bridge may contain lots of root bridges.
               Each Host bridge have PCIE_MAX_ROOTBRIDGE root bridges
               PcieRootBridgeMask have PCIE_MAX_ROOTBRIDGE*HostBridgeNum bits,
               and each bit stands for this PCIe Port is enable or not
            */
            if (!(((( PcieRootBridgeMask >> (PCIE_MAX_ROOTBRIDGE * HostBridgeNum))) >> Port) & 0x1)) {
                continue;
            }

            Status = PciePortInit(soctype, HostBridgeNum, &gastr_pcie_driver_cfg[Port]);
            if(EFI_ERROR(Status))
            {
                DEBUG((EFI_D_ERROR, "HostBridge %d, Pcie Port %d Init Failed! \n", HostBridgeNum, Port));
            }

            InitAtu (&mResAppeture[HostBridgeNum][Port]);

        }
    }


    return EFI_SUCCESS;

}


