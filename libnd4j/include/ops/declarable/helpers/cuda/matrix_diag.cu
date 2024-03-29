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
// Created by GS <sgazeos@gmail.com> on 3/21/2018.
//

#include "ResultSet.h"
#include <ops/declarable/helpers/matrix_diag.h>
#include <Status.h>
#include <ShapeUtils.h>
#include <ShapeUtils.h>
#include <TAD.h>
#include <cuda_exception.h>
#include <helpers/ConstantTadHelper.h>

namespace nd4j {
namespace ops {
namespace helpers {


    template <typename T>
    static __global__ void matrixDiagKernel(void const* inputBuffer, void* outputBuffer, Nd4jLong numTads, Nd4jLong inputLength,
                                       Nd4jLong* tadOnlyInputShapeInfo,  Nd4jLong *tadInputOffsets,
                                       Nd4jLong* tadOnlyOutputShapeInfo, Nd4jLong *tadOutputOffsets) {
        int totalThreads = blockDim.x;
        for (Nd4jLong i = blockIdx.x; i < numTads; i += gridDim.x) {
            auto yOffset = tadInputOffsets[i];
            auto xOffset = tadOutputOffsets[i];
            for (Nd4jLong j = threadIdx.x; j < inputLength; j += totalThreads) {
                Nd4jLong coords[2] = {j, j};
                Nd4jLong tadOffset = shape::getOffset(0, shape::shapeOf(tadOnlyOutputShapeInfo), shape::stride(tadOnlyOutputShapeInfo), coords, 2);
                //shape::getIndexOffset(j, tadOnlyOutputShapeInfo, inputLength)
                *(reinterpret_cast<T*>(outputBuffer) + xOffset + tadOffset) = *(reinterpret_cast<T const*>(inputBuffer) + yOffset + shape::getIndexOffset(j, tadOnlyInputShapeInfo, inputLength));
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
    // Returns a batched matrix tensor with new batched diagonal values.
    // for detailed explanations please take a look on web page: https://www.tensorflow.org/api_docs/python/tf/matrix_set_diag

    template <typename T>
    static int _matrixDiag(nd4j::LaunchContext * context, const NDArray* input, NDArray* output) {
        cudaStream_t* stream = context->getCudaStream();
        //auto listOut  = output->allTensorsAlongDimension({output->rankOf() - 2, output->rankOf() - 1});
        //auto listDiag = input->allTensorsAlongDimension({input->rankOf() - 1});

        //auto repeatDelta = shape::prodLong(newShape.data(), rank) / this->lengthOf();
        std::vector<int> dimsToExclude = ShapeUtils::evalDimsToExclude(input->rankOf(), {input->rankOf() - 1});
        const Nd4jLong numTads = ShapeUtils::getNumOfSubArrs(input->getShapeInfo(), dimsToExclude); //this->tensorsAlongDimension({dimension});
        //printf("Repeat delta %lld, numTads %lld\n", repeatDelta, numTads);
        //tadOnlyInputShapeInfo, tadInputOffsets, tadOnlyOutputShapeInfo, tadOutputOffsets;
        std::vector<int> inputDims({input->rankOf() - 1});
        std::vector<int> outputDims({output->rankOf() - 2, output->rankOf() - 1});

        auto packX = nd4j::ConstantTadHelper::getInstance()->tadForDimensions(input->getShapeInfo(), inputDims);
        auto packZ = nd4j::ConstantTadHelper::getInstance()->tadForDimensions(output->getShapeInfo(), outputDims);

        if (!input->isActualOnDeviceSide())
            input->syncToDevice();

        if (!output->isActualOnDeviceSide())
            output->syncToDevice();

        // create cuda stream and LaunchContext
        cudaError_t cudaResult;

        dim3 launchDims(256, 512, 8192);
        matrixDiagKernel<T><<<launchDims.x, launchDims.y, launchDims.z, *stream>>>(input->getSpecialBuffer(), output->getSpecialBuffer(), numTads, input->sizeAt(-1), packX.specialShapeInfo(), packX.specialOffsets(), packZ.specialShapeInfo(), packZ.specialOffsets());

        return Status::OK();
    }

    int matrixDiag(nd4j::LaunchContext * context, const NDArray* input, NDArray* output) {
        BUILD_SINGLE_SELECTOR(input->dataType(), return _matrixDiag, (context, input, output), LIBND4J_TYPES);
    }

    BUILD_SINGLE_TEMPLATE(template int _matrixDiag, (nd4j::LaunchContext * context, const NDArray* input, NDArray* output), LIBND4J_TYPES);

}
}
}