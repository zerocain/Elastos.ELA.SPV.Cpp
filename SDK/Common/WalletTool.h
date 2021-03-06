// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_WALLET_TOOL_H__
#define __ELASTOS_SDK_WALLET_TOOL_H__

#include "CMemBlock.h"

namespace Elastos {
	namespace ElaWallet {
		class WalletTool {

		public:
			static CMemBlock<char>
			GeneratePhraseFromSeed(const CMBlock &seed, const std::vector<std::string> &WordList);

		};
	}
}

#endif //__ELASTOS_SDK_WALLET_TOOL_H__
