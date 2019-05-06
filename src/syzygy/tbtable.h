/*
	This file is part of Vajolet.
	

    Vajolet is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Vajolet is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Vajolet.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef TBTABLE_H
#define TBTABLE_H

#include "hashKey.h"
#include "tbtypes.h"
#include "tbfile.h"



// struct TBTable contains indexing information to access the corresponding TBFile.
// There are 2 types of TBTable, corresponding to a WDL or a DTZ file. TBTable
// is populated at init time but the nested PairsData records are populated at
// first access, when the corresponding file is memory mapped.
// todo readd template
/*template<TBType Type>*/
class TBTable {
private:
	TBFile _file;
	HashKey _key;
	HashKey _key2;
	unsigned int _pieceCount;
	unsigned int _pawnCount[2]; // [Lead color / other color]
	bool _hasPawns;
	bool _hasUniquePieces;
public:
	explicit TBTable(const std::string& code);

	//typedef typename std::conditional<Type == WDL, WDLScore, int>::type Ret;

	//static constexpr int Sides = Type == WDL ? 2 : 1;


	//std::atomic_bool ready;
	//void* baseAddress;
	//uint8_t* map;
	//uint64_t mapping;
	
	//PairsData items[Sides][4]; // [wtm / btm][FILE_A..FILE_D or 0]

	/*PairsData* get(int stm, int f) {
		return &items[stm % Sides][hasPawns ? f : 0];
	}*/

	// maybe this could be done private or removed
	//TBTable() : ready(false), baseAddress(nullptr) {}
	//explicit TBTable(const std::string& code);
	//explicit TBTable(const TBTable<WDL>& wdl);

	/*~TBTable() {
		if (baseAddress)
		{
			TBFile::unmap(baseAddress, mapping);
		}
	}*/
	
	const HashKey& getKey() const { return _key; };
	const HashKey& getKey2() const { return _key2; };
	unsigned int getPieceCount() const { return _pieceCount; };
	unsigned int getPawnCount(unsigned int x) const { assert(x<2); return _pawnCount[x];};
	bool hasPawns() const { return _hasPawns; };
	bool hasUniquePieces() const { return _hasUniquePieces; };
};

#endif