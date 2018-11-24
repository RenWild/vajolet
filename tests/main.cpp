#include "gtest/gtest.h"

#include <iostream>
#include "./../io.h"
#include "./../bitops.h"
#include "./../data.h"
#include "./../position.h"
#include "./../movegen.h"
#include "../hashKey.h"

using ::testing::Environment;

class EnvironmentInvocationCatcher : public Environment {
protected:
	virtual void SetUp()
	{
		initData();
		HashKey::init();
		Position::initScoreValues();
		Position::initCastleRightsMask();
		Movegen::initMovegenConstant();
	}

	virtual void TearDown()
	{
	}
};


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment( new EnvironmentInvocationCatcher);
  return RUN_ALL_TESTS();
}