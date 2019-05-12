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

#ifndef TBPAIRS_H
#define TBPAIRS_H

#include <vector>
#include <array>

#include "bitBoardIndex.h"
#include "LR.h"
#include "sparseEntry.h"
#include "tbtypes.h"

// struct PairsData contains low level indexing information to access TB data.
// There are 8, 4 or 2 PairsData records for each TBTable, according to type of
// table and if positions have pawns or not. It is populated at first access.
class PairsData {
private:

    uint8_t _flags;                 // Table flags, see enum TBFlag
    uint8_t _maxSymLen;             // Maximum length in bits of the Huffman symbols
    uint8_t _minSymLen;             // Minimum length in bits of the Huffman symbols
    uint32_t _blocksNum;            // Number of blocks in the TB file
    size_t _sizeofBlock;            // Block size in bytes
    size_t _span;                   // About every span values there is a SparseIndex[] entry
    Sym* _lowestSym;                // lowestSym[l] is the symbol of length l with the lowest value
    LR* _btree;                     // btree[sym] stores the left and right symbols that expand sym
    uint16_t* _blockLength;         // Number of stored positions (minus one) for each block: 1..65536
    uint32_t _blockLengthSize;      // Size of blockLength[] table: padded so it's bigger than blocksNum
    SparseEntry* _sparseIndex;      // Partial indices into blockLength[]
    size_t _sparseIndexSize;        // Size of SparseIndex[] table
    uint8_t* _data;                 // Start of Huffman compressed data
    std::vector<uint64_t> _base64;  // base64[l - min_sym_len] is the 64bit-padded lowest symbol of length l
    std::vector<uint8_t> _symlen;   // Number of values (-1) represented by a given Huffman symbol: 1..256
		// todo pay attenction that vajolet piece encoding is different from stockfish one
    std::array<bitboardIndex, TBPIECES> _pieces;// Position pieces: the order of pieces defines the groups
    std::array<uint64_t, TBPIECES+1> _groupIdx; // Start index used for the encoding of the group's pieces
    std::array<int, TBPIECES+1> _groupLen;      // Number of pieces in a given group: KRKN -> (3, 1)
    std::array<uint16_t, 4> _map_idx;           // WDLWin, WDLLoss, WDLCursedWin, WDLBlessedLoss (used in DTZ)
	
	static bitboardIndex _tbPieceConvert(uint8_t rawData);

public:
	PairsData() {}
    ~PairsData(){}
    PairsData(const PairsData& other) = delete;
    PairsData(PairsData&& other) noexcept = delete;// move constructor
    PairsData& operator=(const PairsData& other) =delete; // copy assignment
    PairsData& operator=(PairsData&& other) noexcept = delete; // move assignment
	
	void setPiece(unsigned int idx, uint8_t rawData);
	bitboardIndex getPiece(unsigned int idx) const;
};

#endif