// ==========================================================================
//                                  lambda
// ==========================================================================
// Copyright (c) 2013-2017, Hannes Hauswedell <h2 @ fsfe.org>
// Copyright (c) 2016-2017, Knut Reinert and Freie Universität Berlin
// All rights reserved.
//
// This file is part of Lambda.
//
// Lambda is Free Software: you can redistribute it and/or modify it
// under the terms found in the LICENSE[.md|.rst] file distributed
// together with this file.
//
// Lambda is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// ==========================================================================
// match.h: Main File for the match class
// ==========================================================================

#ifndef SEQAN_LAMBDA_FINDER_H_
#define SEQAN_LAMBDA_FINDER_H_

// #include <forward_list>
// #include <unordered_map>
#include <vector>

#include "options.hpp"

using namespace seqan;

//-----------------------------------------------------------------------------
//                  Finder
//-----------------------------------------------------------------------------

template<typename TAlph>
struct Match
{
    typedef SizeTypeNum_<TAlph>    TQId;
    typedef SizeTypeNum_<TAlph>    TSId;
    typedef SizeTypePos_<TAlph>    TPos;

    TQId qryId;
    TSId subjId;
    TPos qryStart;
//     TPos qryEnd;

    TPos subjStart;
//     TPos subjEnd;

//     Match()
//     :
//         qryId(0), qryStart(0), /*qryEnd(0),*/ subjId(0), subjStart(0)/*, subjEnd(0)*/
//     {
//     }
// 
//     Match(Match const & m2)
//     {
//         qryId       = m2.qryId;
//         qryStart    = m2.qryStart;
//         /*qryEnd      = m2.qryEnd;*/
//         subjId      = m2.subjId;
//         subjStart   = m2.subjStart;
//         /*subjEnd     = m2.subjEnd;*/
//     }

    inline bool operator== (Match const & m2) const
    {
         return std::tie(qryId, subjId, qryStart, subjStart/*, qryEnd, subjEnd*/)
             == std::tie(m2.qryId, m2.subjId, m2.qryStart, m2.subjStart/*, m2.qryEnd, m2.subjEnd*/);
    }
    inline bool operator< (Match const & m2) const
    {
         return std::tie(qryId, subjId, qryStart, subjStart/*, qryEnd, subjEnd*/)
              < std::tie(m2.qryId, m2.subjId, m2.qryStart, m2.subjStart/*, m2.qryEnd, m2.subjEnd*/);
    }
};

template <typename TAlph>
inline void
setToSkip(Match<TAlph> & m)
{
    using TPos          = typename Match<TAlph>::TPos;
    constexpr TPos posMax = std::numeric_limits<TPos>::max();
    m.qryStart = posMax;
    m.subjStart = posMax;
}

template <typename TAlph>
inline bool
isSetToSkip(Match<TAlph> const & m)
{
    using TPos          = typename Match<TAlph>::TPos;
    constexpr TPos posMax = std::numeric_limits<TPos>::max();
    return (m.qryStart == posMax) && (m.subjStart == posMax);
}

// struct IdPairHash
// {
//     std::size_t
//     operator()(std::pair<Match::TQId, Match::TSId> const & pair) const
//     {
//         return std::hash<Match::TQId>()(std::get<0>(pair)) ^
//                std::hash<Match::TSId>()(std::get<1>(pair));
//     }
// };
// 
// 
// // this is a comparison structure that takes the relative abundance of hits
// // on a specific subject into account thereby moving those subjects to
// // front of a queries hits that are more likely to be top-scoring
// struct MatchSortComp :
//     public ::std::binary_function <Match, Match, bool>
// {
//     std::unordered_map<std::pair<Match::TQId, Match::TSId>,
//                        uint64_t,
//                        IdPairHash> map;
// 
//     MatchSortComp(std::vector<Match> const & matches)
//     {
//         for (Match const & m : matches)
//             ++map[std::make_pair(m.qryId, m.subjId)];
//     }
// 
//     inline bool operator() (Match const & i, Match const & j) const
//     {
//         int64_t iAb = -map.at(std::make_pair(i.qryId, i.subjId));
//         int64_t jAb = -map.at(std::make_pair(j.qryId, j.subjId));
//         return std::tie(i.qryId,
//                         iAb,
//                         i.qryStart,
//                         i.subjStart)
//            <   std::tie(j.qryId,
//                         jAb,
//                         j.qryStart,
//                         j.subjStart);
//     }
// };

template <typename TGH, typename TAlph>
inline void
myHyperSortSingleIndex(std::vector<Match<TAlph>> & matches,
                       bool const doubleIndexing,
                       TGH const &)
{
    using TId = typename Match<TAlph>::TQId;

    // regular sort
    std::sort(matches.begin(), matches.end());

    //                    trueQryId, begin,    end
    std::vector<std::tuple<TId, TId, TId>> intervals;
    for (TId i = 1; i <= length(matches); ++i)
    {
        if ((i == length(matches))                      ||
            (matches[i-1].qryId != matches[i].qryId)    ||
            (matches[i-1].subjId / sNumFrames(TGH::blastProgram)) !=
             (matches[i].subjId / sNumFrames(TGH::blastProgram)))
        {
            if (length(intervals) == 0) // first interval
                intervals.emplace_back(std::make_tuple(matches[i-1].qryId
                                                       / qNumFrames(TGH::blastProgram),
                                                       0,
                                                       i));
            else
                intervals.emplace_back(std::make_tuple(matches[i-1].qryId
                                                       / qNumFrames(TGH::blastProgram),
                                                       std::get<2>(intervals.back()),
                                                       i));
        }
    }

    if (doubleIndexing)
    {
        // sort by trueQryId, then lengths of interval
        std::sort(intervals.begin(), intervals.end(),
                [] (std::tuple<TId, TId, TId> const & i1,
                    std::tuple<TId, TId, TId> const & i2)
        {
            return (std::get<0>(i1) != std::get<0>(i2))
                    ? (std::get<0>(i1) < std::get<0>(i2))
                    : ((std::get<2>(i1) - std::get<1>(i1))
                     > (std::get<2>(i2) - std::get<1>(i2)));
        });
    } else
    {
        // sort by lengths of interval, since trueQryId is the same anyway
        std::sort(intervals.begin(), intervals.end(),
                [] (std::tuple<TId, TId, TId> const & i1,
                    std::tuple<TId, TId, TId> const & i2)
        {
            return (std::get<2>(i1) - std::get<1>(i1))
                >  (std::get<2>(i2) - std::get<1>(i2));
        });
    }

    std::vector<Match<TAlph>> tmpVector;
    tmpVector.resize(matches.size());

    TId newIndex = 0;
    for (auto const & i : intervals)
    {
        TId limit = std::get<2>(i);
        for (TId j = std::get<1>(i); j < limit; ++j)
        {
            tmpVector[newIndex] = matches[j];
            newIndex++;
        }
    }
    std::swap(tmpVector, matches);
}


// inline bool
// overlap(Match const & m1, Match const & m2, unsigned char d = 0)
// {
//     SEQAN_ASSERT_EQ(m1.qryId,  m2.qryId);
//     SEQAN_ASSERT_EQ(m1.subjId, m2.subjId);
// 
// //     if (     //match beginnings overlap
// //         ((m1.qryStart  >= _protectUnderflow(m2.qryStart, d)) &&
// //          (m1.qryStart  <= _protectOverflow( m2.qryEnd,   d)) &&
// //          (m1.subjStart >= _protectUnderflow(m2.subjStart,d)) &&
// //          (m1.subjStart <= _protectOverflow( m2.subjEnd,  d))
// //         ) || // OR match endings overlap
// //         ((m1.qryEnd  >= _protectUnderflow(m2.qryStart, d)) &&
// //          (m1.qryEnd  <= _protectOverflow( m2.qryEnd,   d)) &&
// //          (m1.subjEnd >= _protectUnderflow(m2.subjStart,d)) &&
// //          (m1.subjEnd <= _protectOverflow( m2.subjEnd,  d))
// //         )
// //        )
// //         return true;
// 
//     //DEBUG DEBUG DEBUG TODO TODO TODO WHEN ALIGNMENT IS FIXED
//     // also add check if diffference of overlaps is  <= maximum number of gaps
//     if ((intervalOverlap(m1.qryStart, m1.qryEnd, m2.qryStart, m2.qryEnd) > -d) &&
//         (intervalOverlap(m1.subjStart, m1.subjEnd, m2.subjStart, m2.subjEnd) > -d) &&
//         ((m1.qryStart < m2.qryStart ) == (m1.subjStart < m2.subjStart))) // same order
//         return true;
// 
//     return false;
// }

// inline bool contains(Match const & m1, Match const & m2)
// {
// //     if (m1.qryId != m2.qryId)
// //         return false;
//     SEQAN_ASSERT_EQ(m1.qryId,  m2.qryId);
//     if (m1.qryStart != m2.qryStart)
//         return false;
// 
//     if (m1.qryStart > m2.qryStart)
//         return false;
//     if (m1.subjStart > m2.subjStart)
//         return false;
//     if (m1.qryEnd < m2.qryEnd)
//         return false;
//     if (m1.subjEnd < m2.subjEnd)
//         return false;
//     return true;
// }
// 
// inline void
// mergeUnto(Match & m1, Match const & m2)
// {
//     SEQAN_ASSERT_EQ(m1.qryId,  m2.qryId);
//     SEQAN_ASSERT_EQ(m1.subjId, m2.subjId);
//     m1.qryStart  = std::min(m1.qryStart, m2.qryStart);
//     m1.qryEnd    = std::max(m1.qryEnd,   m2.qryEnd);
//     m1.subjStart = std::min(m1.subjStart,m2.subjStart);
//     m1.subjEnd   = std::max(m1.subjEnd,  m2.subjEnd);
// }


// inline void
// _printMatch(Match const & m)
// {
//     std::cout << "MATCH  Query " << m.qryId
//               << "(" << m.qryStart << ", " << m.qryEnd
//               << ")   on Subject "<< m.subjId
//               << "(" << m.subjStart << ", " << m.subjEnd
//               << ")" <<  std::endl << std::flush;
// }





#endif // header guard
