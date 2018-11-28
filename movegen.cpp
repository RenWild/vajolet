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


#include <functional>
#include <list>

#include "bitops.h"
#include "data.h"
#include "movegen.h"
#include "position.h"
#include "vajolet.h"


bitMap Movegen::_KNIGHT_MOVE[squareNumber];
bitMap Movegen::_KING_MOVE[squareNumber];
bitMap Movegen::_PAWN_ATTACK[2][squareNumber];
std::array<bitMap,9> Movegen::_castlePath;

bool Movegen::_isValidCoordinate( const int tofile, const int torank )
{
	return (tofile >= 0) & (tofile <= 7) & (torank >= 0) & (torank <= 7);
}

void Movegen::_setBit( bitMap& b, int file, int rank )
{
	if( _isValidCoordinate( file, rank ) )
	{
		b |= bitSet(BOARDINDEX[file][rank]);
	}
}

void Movegen::initMovegenConstant(void){

	for( auto& x : _castlePath )
	{
		x = 0;
	}
	_castlePath.at( wCastleOO  ) = bitSet(F1) | bitSet(G1);
	_castlePath.at( wCastleOOO ) = bitSet(D1) | bitSet(C1) | bitSet(B1);
	_castlePath.at( bCastleOO  ) = bitSet(F8) | bitSet(G8);
	_castlePath.at( bCastleOOO ) = bitSet(D8) | bitSet(C8) | bitSet(B8);

	initmagicmoves();
	
	struct coord{ int x; int y;};
	std::list<coord> pawnsAttack[2] ={{{-1,1},{1,1}},{{-1,-1},{1,-1}}};
	std::list<coord> knightAttack ={{-2,1},{-1,2},{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1}};
	std::list<coord> kingAttack ={{-1,0},{-1,1},{-1,-1},{0,1},{0,-1},{1,0},{1,-1},{1,1}};
	
	for ( int square = 0; square < squareNumber; ++square )
	{
		int file = FILES[square];
		int rank = RANKS[square];
		
		// pawn attacks
		for( int color = 0; color < 2; ++color)
		{
			_PAWN_ATTACK[color][square] = 0x0;
			for( auto c: pawnsAttack[color] )
			{
				_setBit( _PAWN_ATTACK[color][square], file + c.x, rank + c.y );
			}
		}
		
		// knight moves
		_KNIGHT_MOVE[square] = 0x0;
		for( auto c: knightAttack )
		{
			_setBit( _KNIGHT_MOVE[square], file + c.x, rank + c.y );
		}
		
		// king moves;
		_KING_MOVE[square]= 0x0;
		for( auto c: kingAttack )
		{
			_setBit( _KING_MOVE[square], file + c.x, rank + c.y );
		}
	}
}



template<Movegen::genType type>
void Movegen::generateMoves( MoveList<MAX_MOVE_PER_POSITION>& ml ) const
{

	// initialize constants
	const state &s =_pos.getActualStateConst();
	const bitMap& enemy = _pos.getTheirBitmap(Pieces);
	const bitMap& occupiedSquares = _pos.getOccupationBitmap();

	//divide pawns
	
	const bitMap& seventhRankMask = RANKMASK[ s.isBlackTurn() ? A2:A7];

	bitMap promotionPawns =  _pos.getOurBitmap(Pawns) & seventhRankMask ;
	bitMap nonPromotionPawns =  _pos.getOurBitmap(Pawns)^ promotionPawns;

	const tSquare kingSquare = _pos.getSquareOfOurKing();
	assert(kingSquare<squareNumber);

	// populate the target squares bitmaps
	bitMap kingTarget;
	bitMap target;
	if constexpr (type==Movegen::allEvasionMg)
	{
		assert(s.getCheckers());
		target = ( s.getCheckers() | SQUARES_BETWEEN[kingSquare][ firstOne( s.getCheckers() ) ]) & ~_pos.getOurBitmap(Pieces);
		kingTarget = ~_pos.getOurBitmap(Pieces);
	}
	else if constexpr (type==Movegen::captureEvasionMg)
	{
		assert(s.getCheckers());
		target = ( s.getCheckers() ) & ~_pos.getOurBitmap(Pieces);
		kingTarget = target | _pos.getTheirBitmap(Pieces);
	}
	else if constexpr (type==Movegen::quietEvasionMg)
	{
		assert( s.getCheckers() );
		target = ( SQUARES_BETWEEN[kingSquare] [firstOne( s.getCheckers() ) ]) & ~_pos.getOurBitmap(Pieces);
		kingTarget = ~occupiedSquares;
	}
	else if constexpr (type== Movegen::allNonEvasionMg)
	{
		target= ~_pos.getOurBitmap(Pieces);
		kingTarget= target;
	}
	else if constexpr (type== Movegen::captureMg)
	{
		target = _pos.getTheirBitmap(Pieces);
		kingTarget = target;
	}
	else if constexpr (type== Movegen::quietMg)
	{
		target = ~occupiedSquares;
		kingTarget = target;
	}
	else if constexpr (type== Movegen::quietChecksMg)
	{
		target = ~occupiedSquares;
		kingTarget = target;
	}else
	{
		assert(false);
		assert(s.getCheckers());
		target = ( s.getCheckers() | SQUARES_BETWEEN[kingSquare][ firstOne( s.getCheckers() ) ]) & ~_pos.getOurBitmap(Pieces);
		kingTarget = ~_pos.getOurBitmap(Pieces);
	}


	

	//------------------------------------------------------
	// king
	//------------------------------------------------------
	bitboardIndex piece = s.getKingOfActivePlayer();
	generateKingMoves<type>( ml, kingSquare, occupiedSquares, kingTarget, enemy );
	
	// if the king is in check from 2 enemy, it can only run away, we should not search any other move
	if((type == Movegen::allEvasionMg || type == Movegen::captureEvasionMg || type == Movegen::quietEvasionMg) && s.isInDoubleCheck() )
	{
		return;
	}
	//------------------------------------------------------
	// queen
	//------------------------------------------------------
	generatePieceMoves<type>( ml, &_attackFromQueen, ++piece, kingSquare, occupiedSquares, target, enemy );
	
	//------------------------------------------------------
	// rook
	//------------------------------------------------------
	generatePieceMoves<type>( ml, &_attackFromRook, ++piece, kingSquare, occupiedSquares, target, enemy );
	
	//------------------------------------------------------
	// bishop
	//------------------------------------------------------
	generatePieceMoves<type>( ml, &_attackFromBishop, ++piece, kingSquare, occupiedSquares, target, enemy );

	//------------------------------------------------------
	// knight
	//------------------------------------------------------
	generatePieceMoves<type>( ml, &_attackFromKnight, ++piece, kingSquare, occupiedSquares, target, enemy );

	bitMap moves;
	Move m(Move::NOMOVE);
	//------------------------------------------------------
	// Pawns
	//------------------------------------------------------
	piece++;
	if(type != Movegen::captureMg && type != Movegen::captureEvasionMg)
	{
		bitMap pawnPushed;
		//push
		moves = (s.isBlackTurn()? (nonPromotionPawns>>8):(nonPromotionPawns<<8)) & ~occupiedSquares;
		pawnPushed = moves;
		moves &= target;

		while(moves)
		{
			tSquare to = iterateBit(moves);
			tSquare from = to - pawnPush(s.isBlackTurn());

			m.setTo( to );
			m.setFrom( from );

			if( !s.isPinned( from ) || squaresAligned(from,to,kingSquare))
			{
				if(type !=Movegen::quietChecksMg || _pos.moveGivesCheck(m))
				{
					ml.insert(m);
				}
			}
		}

		//double push
		const bitMap& thirdRankMask = RANKMASK[ s.isBlackTurn() ? A6:A3];
		moves = (s.isBlackTurn()? ((pawnPushed & thirdRankMask)>>8):((pawnPushed & thirdRankMask)<<8)) & ~occupiedSquares & target;

		while(moves)
		{
			tSquare to = iterateBit(moves);
			tSquare from = to - 2*pawnPush(s.isBlackTurn());

			m.setTo( to );
			m.setFrom( from );
			if( !s.isPinned( from ) || squaresAligned(from ,to ,kingSquare))
			{
				if(type !=Movegen::quietChecksMg || _pos.moveGivesCheck(m))
				{
					ml.insert(m);
				}
			}
		}
	}

	int delta;

	if(type!= Movegen::quietMg && type!=Movegen::quietChecksMg && type != Movegen::quietEvasionMg)
	{
		//left capture
		delta = s.isBlackTurn()?-9:7;

		moves = (s.isBlackTurn()?(nonPromotionPawns&(~FILEMASK[A1]))>>9:(nonPromotionPawns&(~FILEMASK[A1]))<<7) & enemy & target;
		while(moves)
		{
			tSquare to = iterateBit(moves);
			tSquare from = (tSquare)(to - delta);

			if( !s.isPinned( from ) || squaresAligned(from,to,kingSquare))
			{
				m.setTo( to );
				m.setFrom( from );
				ml.insert(m);
			}
		}

		//right capture
		delta=s.isBlackTurn()?-7:9;

		moves = (s.isBlackTurn()?(nonPromotionPawns&(~FILEMASK[H1]))>>7:(nonPromotionPawns&(~FILEMASK[H1]))<<9) & enemy & target;
		while(moves)
		{
			tSquare to = iterateBit(moves);
			tSquare from = (tSquare)(to -delta);


			if( !s.isPinned( from ) || squaresAligned(from,to,kingSquare))
			{
				m.setTo( to );
				m.setFrom( from );
				ml.insert(m);
			}
		}
	}

	// PROMOTIONS
	m.setFlag( Move::fpromotion );
	if(type != Movegen::captureMg && type != Movegen::captureEvasionMg)
	{
		moves = (s.isBlackTurn()? (promotionPawns>>8):(promotionPawns<<8))& ~occupiedSquares & target;
		while(moves)
		{
			tSquare to = iterateBit(moves);
			tSquare from = to - pawnPush(s.isBlackTurn());

			m.setTo( to );
			m.setFrom( from );

			if( !s.isPinned( from ) ||	squaresAligned(from,to,kingSquare))
			{
				for(Move::epromotion prom=Move::promQueen; prom<= Move::promKnight; prom=(Move::epromotion)(prom+1))
				{
					m.setPromotion( prom );
					ml.insert(m);
				}
			}
		}
	}

	Color color = s.isBlackTurn()? black : white;

	if( type!= Movegen::quietMg && type!= Movegen::quietChecksMg && type!= Movegen::quietEvasionMg)
	{
		//left capture
		delta = s.isBlackTurn()?-9:7;
		moves = (s.isBlackTurn()?(promotionPawns&(~FILEMASK[A1]))>>9:(promotionPawns&(~FILEMASK[A1]))<<7) & enemy & target;
		while(moves)
		{
			tSquare to = iterateBit(moves);
			tSquare from = (tSquare)(to -delta);

			m.setTo( to );
			m.setFrom( from );

			if( !s.isPinned( from ) || squaresAligned(from,to,kingSquare))
			{
				for(Move::epromotion prom=Move::promQueen;prom<= Move::promKnight; prom=(Move::epromotion)(prom+1))
				{
					m.setPromotion( prom );
					ml.insert(m);
				}
			}
		}

		//right capture
		delta=s.isBlackTurn()?-7:9;
		moves = (s.isBlackTurn()?(promotionPawns&(~FILEMASK[H1]))>>7:(promotionPawns&(~FILEMASK[H1]))<<9) & enemy & target;
		while(moves)
		{

			tSquare to = iterateBit(moves);
			tSquare from = (tSquare)(to -delta);

			m.setTo( to );
			m.setFrom( from );

			if( !s.isPinned( from ) || squaresAligned(from,to,kingSquare))
			{
				for(Move::epromotion prom=Move::promQueen;prom<= Move::promKnight; prom=(Move::epromotion)(prom+1))
				{
					m.setPromotion( prom );
					ml.insert(m);
				}
			}
		}

		m.setPromotion( Move::promQueen );
		m.setFlag( Move::fnone );

		// ep capture

		if( s.hasEpSquare() )
		{
			auto epSquare = s.getEpSquare();
			m.setFlag( Move::fenpassant );
			bitMap epAttacker = nonPromotionPawns & _attackFromPawn( epSquare, 1-color );

			while(epAttacker)
			{
				tSquare from = iterateBit(epAttacker);

				bitMap captureSquare= FILEMASK[ epSquare ] & RANKMASK[from];
				bitMap occ = occupiedSquares^bitSet(from)^bitSet( epSquare )^captureSquare;

				if(	!((_attackFromRook(kingSquare, occ) & (_pos.getTheirBitmap(Queens) | _pos.getTheirBitmap(Rooks))) |
						(Movegen::_attackFromBishop(kingSquare, occ) & (_pos.getTheirBitmap(Queens) | _pos.getTheirBitmap(Bishops))))
				)
				{
					m.setTo( epSquare );
					m.setFrom( from );
					ml.insert(m);
				}
			}

		}
	}

	//king castle
	if(type !=Movegen::allEvasionMg && type!=Movegen::captureEvasionMg && type!=Movegen::quietEvasionMg && type!= Movegen::captureMg)
	{
		m.setPromotion( Move::promQueen );
		if( !s.isInCheck() && s.hasCastleRight( castleOO | castleOOO, color ) )
		{
			eCastle cr = state::calcCastleRight( castleOO, color );
			if( s.hasCastleRight( cr ) && isCastlePathFree( cr ) )
			{

				bool castleDenied = false;
				for( tSquare x = (tSquare)1; x<3; x++)
				{
					assert(kingSquare+x<squareNumber);
					if(_pos.getAttackersTo(kingSquare+x,occupiedSquares) & _pos.getTheirBitmap(Pieces))
					{
						castleDenied = true;
						break;
					}
				}
				if(!castleDenied)
				{
					m.setFlag( Move::fcastle );
					m.setFrom( kingSquare );
					m.setTo( (tSquare)(kingSquare + 2) );
					if(type !=Movegen::quietChecksMg || _pos.moveGivesCheck(m))
					{
						ml.insert(m);
					}
				}


			}
			cr = state::calcCastleRight( castleOOO, color );
			if( s.hasCastleRight( cr ) && isCastlePathFree( cr ) )
			{
				bool castleDenied = false;
				for( tSquare x = (tSquare)1 ;x<3 ;x++)
				{
					assert(kingSquare-x<squareNumber);
					if(_pos.getAttackersTo(kingSquare-x, occupiedSquares) & _pos.getTheirBitmap(Pieces))
					{
						castleDenied = true;
						break;
					}
				}
				if(!castleDenied)
				{
					m.setFlag( Move::fcastle );
					m.setFrom( kingSquare );
					m.setTo( (tSquare)(kingSquare - 2) );
					if(type != Movegen::quietChecksMg || _pos.moveGivesCheck(m))
					{
						ml.insert(m);
					}
				}
			}
		}
	}
}
template void Movegen::generateMoves<Movegen::captureMg>( MoveList<MAX_MOVE_PER_POSITION>& ml ) const;
template void Movegen::generateMoves<Movegen::quietMg>( MoveList<MAX_MOVE_PER_POSITION>& ml ) const;
template void Movegen::generateMoves<Movegen::quietChecksMg>( MoveList<MAX_MOVE_PER_POSITION>& ml ) const;

template<Movegen::genType type>
inline void Movegen::generateKingMoves( MoveList<MAX_MOVE_PER_POSITION>& ml, const tSquare kingSquare, const bitMap occupiedSquares, const bitMap kingTarget, const bitMap enemy )const
{
	Move m(Move::NOMOVE);
	
	m.setFrom( kingSquare );

	bitMap moves = _attackFromKing(kingSquare,occupiedSquares) & kingTarget;

	while(moves)
	{
		tSquare to = iterateBit(moves);
		m.setTo( to );

		if( !(_pos.getAttackersTo(to, occupiedSquares & ~_pos.getOurBitmap(King)) & enemy) )
		{
			if( type !=Movegen::quietChecksMg || _pos.moveGivesCheck( m ) )
			{
				ml.insert(m);
			}
		}
	}
}
template<Movegen::genType type>
inline void Movegen::generatePieceMoves( MoveList<MAX_MOVE_PER_POSITION>& ml, bitMap (*attack)(const tSquare,const bitMap&), const bitboardIndex piece, const tSquare kingSquare, const bitMap occupiedSquares, const bitMap target, const bitMap enemy )const
{
	Move m(Move::NOMOVE);
	bitMap bFrom = _pos.getBitmap(piece);
	while(bFrom)
	{
		tSquare from = iterateBit(bFrom);
		assert(from < squareNumber);
		m.setFrom( from );

		bitMap moves = attack(from,occupiedSquares) & target;

		while(moves)
		{
			tSquare to = iterateBit(moves);
			m.setTo( to );

			if( !_pos.getActualStateConst().isPinned( from ) || squaresAligned(from, to, kingSquare))
			{
				if(type !=Movegen::quietChecksMg || _pos.moveGivesCheck(m))
				{
					ml.insert(m);
				}
			}
		}
	}
}


template<>
void Movegen::generateMoves<Movegen::allMg>( MoveList<MAX_MOVE_PER_POSITION>& ml ) const
{

	if(_pos.isInCheck())
	{
		generateMoves<Movegen::captureEvasionMg>( ml);
		generateMoves<Movegen::quietEvasionMg>( ml );
	}
	else
	{
		generateMoves<Movegen::genType::captureMg>( ml );
		generateMoves<Movegen::genType::quietMg>( ml );
	}

}

bool Movegen::isCastlePathFree( const eCastle c ) const
{
	assert( c < 9);
	return !( _castlePath[c] & _pos.getOccupationBitmap() );
}

