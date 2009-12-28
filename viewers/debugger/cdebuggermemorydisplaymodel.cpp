#include "cdebuggermemorydisplaymodel.h"

CDebuggerMemoryDisplayModel::CDebuggerMemoryDisplayModel(QObject* parent, eMemoryType display)
{
   m_display = display;
   switch ( m_display )
   {
      case eMemory_CPU:
         m_offset = 0;
      break;
      case eMemory_PPUregs:
         m_offset = 0x2000;
      break;
      case eMemory_IOregs:
         m_offset = 0x4000;
      break;
      case eMemory_cartSRAM:
         m_offset = 0x6000;
      break;
      case eMemory_cartEXRAM:
         m_offset = 0x5C00;
      break;
      case eMemory_cartCHRMEM:
         m_offset = 0;
      break;
      case eMemory_PPU:
         m_offset = 0x2000;
      break;
      case eMemory_PPUpalette:
         m_offset = 0x3F00;
      break;
      case eMemory_PPUoam:
         m_offset = 0;
      break;
   }
}

CDebuggerMemoryDisplayModel::~CDebuggerMemoryDisplayModel()
{
}

QVariant CDebuggerMemoryDisplayModel::data(const QModelIndex &index, int role) const
{
   char data [ 64 ];

   if (!index.isValid())
      return QVariant();

   if (role != Qt::DisplayRole)
      return QVariant();

   sprintf ( data, "%02X", (int)index.internalPointer() );
   return data;
}

Qt::ItemFlags CDebuggerMemoryDisplayModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return flags;
}

QVariant CDebuggerMemoryDisplayModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (role != Qt::DisplayRole)
      return QVariant();
   char buffer [ 8 ];
   if ( orientation == Qt::Horizontal )
   {
      sprintf ( buffer, "x%1X", section );
   }
   else
   {
      sprintf ( buffer, "$%04X:", m_offset+(section<<4) );
   }

   return  QString(buffer);
}

QModelIndex CDebuggerMemoryDisplayModel::index(int row, int column, const QModelIndex &parent) const
{
   switch ( m_display )
   {
      case eMemory_CPU:
         return createIndex(row, column, (int)C6502::_MEM(m_offset+((row<<4)|column)));
      break;
      case eMemory_PPUregs:
      break;
      case eMemory_IOregs:
      break;
      case eMemory_cartSRAM:
      break;
      case eMemory_cartEXRAM:
      break;
      case eMemory_cartCHRMEM:
      break;
      case eMemory_PPU:
      return createIndex(row, column, (int)CPPU::_MEM(m_offset+((row<<4)|column)));
      break;
      case eMemory_PPUpalette:
      break;
      case eMemory_PPUoam:
      break;
   }
   return QModelIndex();
}

int CDebuggerMemoryDisplayModel::rowCount(const QModelIndex &parent) const
{
   switch ( m_display )
   {
      case eMemory_CPU:
         return (MEM_2KB>>4);
      break;
      case eMemory_PPUregs:
         return 1;
      break;
      case eMemory_IOregs:
         return 2;
      break;
      case eMemory_cartSRAM:
         return (MEM_8KB>>4);
      break;
      case eMemory_cartEXRAM:
         return (MEM_1KB>>4);
      break;
      case eMemory_cartCHRMEM:
         return (MEM_8KB>>4);
      break;
      case eMemory_PPU:
         return (MEM_2KB>>4);
      break;
      case eMemory_PPUpalette:
         return (MEM_32B>>4);
      break;
      case eMemory_PPUoam:
         return (MEM_256B>>4);
      break;
   }
   return 0;
}

int CDebuggerMemoryDisplayModel::columnCount(const QModelIndex &parent) const
{
   if ( parent.isValid())
   {
      return 0;
   }
   return 16;
}

void CDebuggerMemoryDisplayModel::layoutChangedEvent()
{
    this->layoutChanged();
}
