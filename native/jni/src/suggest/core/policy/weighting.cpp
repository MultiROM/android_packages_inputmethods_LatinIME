/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "suggest/core/policy/weighting.h"

#include "char_utils.h"
#include "defines.h"
#include "hash_map_compat.h"
#include "suggest/core/dicnode/dic_node.h"
#include "suggest/core/dicnode/dic_node_profiler.h"
#include "suggest/core/dicnode/dic_node_utils.h"
#include "suggest/core/session/dic_traverse_session.h"

namespace latinime {

static inline void profile(const CorrectionType correctionType, DicNode *const node) {
#if DEBUG_DICT
    switch (correctionType) {
    case CT_OMISSION:
        PROF_OMISSION(node->mProfiler);
        return;
    case CT_ADDITIONAL_PROXIMITY:
        PROF_ADDITIONAL_PROXIMITY(node->mProfiler);
        return;
    case CT_SUBSTITUTION:
        PROF_SUBSTITUTION(node->mProfiler);
        return;
    case CT_NEW_WORD:
        PROF_NEW_WORD(node->mProfiler);
        return;
    case CT_MATCH:
        PROF_MATCH(node->mProfiler);
        return;
    case CT_COMPLETION:
        PROF_COMPLETION(node->mProfiler);
        return;
    case CT_TERMINAL:
        PROF_TERMINAL(node->mProfiler);
        return;
    case CT_SPACE_SUBSTITUTION:
        PROF_SPACE_SUBSTITUTION(node->mProfiler);
        return;
    case CT_INSERTION:
        PROF_INSERTION(node->mProfiler);
        return;
    case CT_TRANSPOSITION:
        PROF_TRANSPOSITION(node->mProfiler);
        return;
    default:
        // do nothing
        return;
    }
#else
    // do nothing
#endif
}

/* static */ void Weighting::addCostAndForwardInputIndex(const Weighting *const weighting,
        const CorrectionType correctionType,
        const DicTraverseSession *const traverseSession,
        const DicNode *const parentDicNode, DicNode *const dicNode,
        hash_map_compat<int, int16_t> *const bigramCacheMap) {
    const int inputSize = traverseSession->getInputSize();
    DicNode_InputStateG inputStateG;
    inputStateG.mNeedsToUpdateInputStateG = false; // Don't use input info by default
    const float spatialCost = Weighting::getSpatialCost(weighting, correctionType,
            traverseSession, parentDicNode, dicNode, &inputStateG);
    const float languageCost = Weighting::getLanguageCost(weighting, correctionType,
            traverseSession, parentDicNode, dicNode, bigramCacheMap);
    const bool edit = Weighting::isEditCorrection(correctionType);
    const bool proximity = Weighting::isProximityCorrection(weighting, correctionType,
            traverseSession, dicNode);
    profile(correctionType, dicNode);
    if (inputStateG.mNeedsToUpdateInputStateG) {
        dicNode->updateInputIndexG(&inputStateG);
    } else {
        dicNode->forwardInputIndex(0, getForwardInputCount(correctionType),
                (correctionType == CT_TRANSPOSITION));
    }
    dicNode->addCost(spatialCost, languageCost, weighting->needsToNormalizeCompoundDistance(),
            inputSize, edit, proximity);
}

/* static */ float Weighting::getSpatialCost(const Weighting *const weighting,
        const CorrectionType correctionType,
        const DicTraverseSession *const traverseSession, const DicNode *const parentDicNode,
        const DicNode *const dicNode, DicNode_InputStateG *const inputStateG) {
    switch(correctionType) {
    case CT_OMISSION:
        return weighting->getOmissionCost(parentDicNode, dicNode);
    case CT_ADDITIONAL_PROXIMITY:
        // only used for typing
        return weighting->getAdditionalProximityCost();
    case CT_SUBSTITUTION:
        // only used for typing
        return weighting->getSubstitutionCost();
    case CT_NEW_WORD:
        return weighting->getNewWordCost(dicNode);
    case CT_MATCH:
        return weighting->getMatchedCost(traverseSession, dicNode, inputStateG);
    case CT_COMPLETION:
        return weighting->getCompletionCost(traverseSession, dicNode);
    case CT_TERMINAL:
        return weighting->getTerminalSpatialCost(traverseSession, dicNode);
    case CT_SPACE_SUBSTITUTION:
        return weighting->getSpaceSubstitutionCost();
    case CT_INSERTION:
        return weighting->getInsertionCost(traverseSession, parentDicNode, dicNode);
    case CT_TRANSPOSITION:
        return weighting->getTranspositionCost(traverseSession, parentDicNode, dicNode);
    default:
        return 0.0f;
    }
}

/* static */ float Weighting::getLanguageCost(const Weighting *const weighting,
        const CorrectionType correctionType, const DicTraverseSession *const traverseSession,
        const DicNode *const parentDicNode, const DicNode *const dicNode,
        hash_map_compat<int, int16_t> *const bigramCacheMap) {
    switch(correctionType) {
    case CT_OMISSION:
        return 0.0f;
    case CT_SUBSTITUTION:
        return 0.0f;
    case CT_NEW_WORD:
        return weighting->getNewWordBigramCost(traverseSession, parentDicNode, bigramCacheMap);
    case CT_MATCH:
        return 0.0f;
    case CT_COMPLETION:
        return 0.0f;
    case CT_TERMINAL: {
        const float languageImprobability =
                DicNodeUtils::getBigramNodeImprobability(
                        traverseSession->getOffsetDict(), dicNode, bigramCacheMap);
        return weighting->getTerminalLanguageCost(traverseSession, dicNode, languageImprobability);
    }
    case CT_SPACE_SUBSTITUTION:
        return 0.0f;
    case CT_INSERTION:
        return 0.0f;
    case CT_TRANSPOSITION:
        return 0.0f;
    default:
        return 0.0f;
    }
}

/* static */ bool Weighting::isEditCorrection(const CorrectionType correctionType) {
    switch(correctionType) {
        case CT_OMISSION:
            return true;
        case CT_ADDITIONAL_PROXIMITY:
            // Should return true?
            return false;
        case CT_SUBSTITUTION:
            // Should return true?
            return false;
        case CT_NEW_WORD:
            return false;
        case CT_MATCH:
            return false;
        case CT_COMPLETION:
            return false;
        case CT_TERMINAL:
            return false;
        case CT_SPACE_SUBSTITUTION:
            return false;
        case CT_INSERTION:
            return true;
        case CT_TRANSPOSITION:
            return true;
        default:
            return false;
    }
}

/* static */ bool Weighting::isProximityCorrection(const Weighting *const weighting,
        const CorrectionType correctionType,
        const DicTraverseSession *const traverseSession, const DicNode *const dicNode) {
    switch(correctionType) {
        case CT_OMISSION:
            return false;
        case CT_ADDITIONAL_PROXIMITY:
            return false;
        case CT_SUBSTITUTION:
            return false;
        case CT_NEW_WORD:
            return false;
        case CT_MATCH:
            return weighting->isProximityDicNode(traverseSession, dicNode);
        case CT_COMPLETION:
            return false;
        case CT_TERMINAL:
            return false;
        case CT_SPACE_SUBSTITUTION:
            return false;
        case CT_INSERTION:
            return false;
        case CT_TRANSPOSITION:
            return false;
        default:
            return false;
    }
}

/* static */ int Weighting::getForwardInputCount(const CorrectionType correctionType) {
    switch(correctionType) {
        case CT_OMISSION:
            return 0;
        case CT_ADDITIONAL_PROXIMITY:
            return 0;
        case CT_SUBSTITUTION:
            return 0;
        case CT_NEW_WORD:
            return 0;
        case CT_MATCH:
            return 1;
        case CT_COMPLETION:
            return 0;
        case CT_TERMINAL:
            return 0;
        case CT_SPACE_SUBSTITUTION:
            return 1;
        case CT_INSERTION:
            return 2;
        case CT_TRANSPOSITION:
            return 2;
        default:
            return 0;
    }
}
}  // namespace latinime