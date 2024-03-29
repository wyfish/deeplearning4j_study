/*******************************************************************************
 * Copyright (c) 2015-2018 Skymind, Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

//
// Created by george@skymind.io on 6/1/2018.
// @author Yurii Shyrma (iuriish@yahoo.com)
//

#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/helpers/transforms.h>
#include <ops/declarable/helpers/axis.h>

namespace nd4j {
namespace ops {

#if NOT_EXCLUDED(OP_reduce_max)

//////////////////////////////////////////////////////////////////////////
CUSTOM_OP_IMPL(reduce_max, 1, 1, false, 0, 0) {

    auto input = INPUT_VARIABLE(0);
    auto output = OUTPUT_VARIABLE(0);

    std::vector<int> dimensions = *block.getIArguments();

    if (block.width() > 1) {
        auto axesVector = INPUT_VARIABLE(1);
        helpers::adjustAxis(input->rankOf(), axesVector, dimensions);
    }

    REQUIRE_TRUE(dimensions.size() <= input->rankOf(), 0, "REDUCE_MAX OP: the number of dimensions to reduce along must be <= input array rank, but got %i instead" , dimensions.size());

    for(const auto& item : dimensions)
        REQUIRE_TRUE(item >= -input->shapeInfo()[0] && item < input->shapeInfo()[0], 0, "REDUCE_MAX OP: the input dimension to reduce along must be in range [-%i, %i), but got %i instead !" , input->rankOf(), input->rankOf(), item);

    bool keepDims = false;//: false;
    if (block.getBArguments()->size() > 0)
        keepDims = B_ARG(0);
    else if (block.getTArguments()->size() > 0)
        keepDims = (bool)T_ARG(0);

    input->reduceAlongDimension(reduce::Max, output, dimensions, keepDims);

    return Status::OK();
}

DECLARE_SHAPE_FN(reduce_max) {

    bool keepDims = false;//: false;

    if (block.getBArguments()->size() > 0)
        keepDims = B_ARG(0);
    else if (block.getTArguments()->size() > 0)
        keepDims = (bool)T_ARG(0);

    auto dimensions = *block.getIArguments();
    if (block.width() > 1) {
        auto axesVector = INPUT_VARIABLE(1);
        helpers::adjustAxis(INPUT_VARIABLE(0)->rankOf(), axesVector, dimensions);
    }

    REQUIRE_TRUE(dimensions.size() <= inputShape->at(0)[0], 0, "REDUCE_MAX OP: the number of dimensions to reduce along must be <= input array rank, but got %i instead" , dimensions.size());

    for(const auto& item : dimensions)
        REQUIRE_TRUE(item >= -inputShape->at(0)[0] && item < inputShape->at(0)[0], 0, "REDUCE_MAX OP: the input dimension to reduce along must be in range [-%i, %i), but got %i instead !" , inputShape->at(0)[0], inputShape->at(0)[0], item);

    Nd4jLong* outShapeInfo = ShapeUtils::evalReduceShapeInfo(shape::order(inputShape->at(0)), dimensions, inputShape->at(0), keepDims, false, block.getWorkspace());

    return SHAPELIST(outShapeInfo);
}

DECLARE_TYPES(reduce_max) {
    getOpDescriptor()
        ->setAllowedInputTypes(nd4j::DataType::ANY)
        ->setSameMode(true);
}

#endif 
#if NOT_EXCLUDED(OP_reduce_max_bp)

//////////////////////////////////////////////////////////////////////////
CUSTOM_OP_IMPL(reduce_max_bp, 2, 1, false, 0, 0) {

    auto input = INPUT_VARIABLE(0);
    auto gradO = INPUT_VARIABLE(1);
    auto gradI = OUTPUT_VARIABLE(0);

    std::vector<int> dimensions = *block.getIArguments();

    if (block.width() > 2) {
        auto axesVector = INPUT_VARIABLE(2);
        helpers::adjustAxis(input->rankOf(), axesVector, dimensions);
    }

    REQUIRE_TRUE(dimensions.size() <= input->rankOf(), 0, "REDUCE_MAX_BP OP: the number of dimensions to reduce along must be <= input array rank, but got %i instead" , dimensions.size());

    for(const auto& item : dimensions)
        REQUIRE_TRUE(item >= -input->shapeInfo()[0] && item < input->shapeInfo()[0], 0, "REDUCE_MAX_BP OP: the input dimension to reduce along must be in range [-%i, %i), but got %i instead !" , input->rankOf(), input->rankOf(), item);

    // *** calculations *** //

    *gradI = 0;

    if(gradO->lengthOf() == 1) {

        auto indOfMaxElem = input->indexReduceNumber(nd4j::indexreduce::IndexMax);
        gradI->p(indOfMaxElem.t<Nd4jLong>(0), gradO->e(0));
    }
    else {

        auto indicesArr = input->applyIndexReduce(nd4j::indexreduce::IndexMax, dimensions);
        helpers::scatterSimple(6, *gradI, *gradO, *indicesArr, ShapeUtils::evalDimsToExclude(gradI->rankOf(), dimensions)); // 6 corresponds to copy operation
        delete indicesArr;
    }

    return Status::OK();
}


DECLARE_SHAPE_FN(reduce_max_bp) {

    std::vector<int> dimensions = *block.getIArguments();

    if (block.width() > 2) {
        auto axesVector = INPUT_VARIABLE(2);
        helpers::adjustAxis(INPUT_VARIABLE(0)->rankOf(), axesVector, dimensions);
    }

    REQUIRE_TRUE(dimensions.size() <= inputShape->at(0)[0], 0, "REDUCE_MAX_BP OP: the number of dimensions to reduce along must be <= input array rank, but got %i instead" , dimensions.size());

    for(const auto& item : dimensions)
        REQUIRE_TRUE(item >= -inputShape->at(0)[0] && item < inputShape->at(0)[0], 0, "REDUCE_MAX_BP OP: the input dimension to reduce along must be in range [-%i, %i), but got %i instead !", inputShape->at(0)[0], inputShape->at(0)[0], item);

    Nd4jLong* outShapeInfo;
    COPY_SHAPE(inputShape->at(0), outShapeInfo);

    return SHAPELIST(CONSTANT(outShapeInfo));
}

DECLARE_TYPES(reduce_max_bp) {
    getOpDescriptor()
        ->setAllowedInputTypes(nd4j::DataType::ANY)
        ->setAllowedOutputTypes({ALL_FLOATS});
}

#endif

}
}
