/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Siva Rajamanickam (srajama@sandia.gov)
//
// ************************************************************************
//@HEADER
*/
#ifndef _KOKKOS_SPGEMM_HPP
#define _KOKKOS_SPGEMM_HPP

#include "KokkosSparse_spgemm_numeric.hpp"
#include "KokkosSparse_spgemm_symbolic.hpp"
#include "KokkosSparse_spgemm_jacobi.hpp"

namespace KokkosSparse {

template <class KernelHandle, class AMatrix, class BMatrix, class CMatrix>
void spgemm_symbolic(KernelHandle& kh, const AMatrix& A, const bool Amode,
                     const BMatrix& B, const bool Bmode, CMatrix& C) {
  using row_map_type = typename CMatrix::row_map_type::non_const_type;
  using entries_type = typename CMatrix::index_type::non_const_type;
  using values_type  = typename CMatrix::values_type::non_const_type;

  row_map_type row_mapC(
      Kokkos::view_alloc(Kokkos::WithoutInitializing, "non_const_lnow_row"),
      A.numRows() + 1);
  entries_type entriesC;
  values_type valuesC;

  KokkosSparse::Experimental::spgemm_symbolic(
      &kh, A.numRows(), B.numRows(), B.numCols(), A.graph.row_map,
      A.graph.entries, Amode, B.graph.row_map, B.graph.entries, Bmode,
      row_mapC);

  const size_t c_nnz_size = kh.get_spgemm_handle()->get_c_nnz();
  if (c_nnz_size) {
    entriesC = entries_type(
        Kokkos::view_alloc(Kokkos::WithoutInitializing, "entriesC"),
        c_nnz_size);
    valuesC = values_type(
        Kokkos::view_alloc(Kokkos::WithoutInitializing, "valuesC"), c_nnz_size);
  }

  C = CMatrix("C=AB", A.numRows(), B.numCols(), c_nnz_size, valuesC, row_mapC,
              entriesC);
}

// Symbolic phase for block SpGEMM (BSR matrices)
template <class KernelHandle, class AMatrixType, class BMatrixType,
          class CMatrixType>
void block_spgemm_symbolic(KernelHandle& kh, const AMatrixType& A,
                           const bool transposeA, const BMatrixType& B,
                           const bool transposeB, CMatrixType& C) {
  using row_map_type = typename CMatrixType::row_map_type::non_const_type;
  using entries_type = typename CMatrixType::index_type::non_const_type;
  using values_type  = typename CMatrixType::values_type::non_const_type;

  auto blockDim = A.blockDim();
  if (blockDim != B.blockDim()) {
    throw std::invalid_argument(
        "Block SpGEMM must be called for matrices with the same block size");
  }

  row_map_type row_mapC(
      Kokkos::view_alloc(Kokkos::WithoutInitializing, "non_const_lnow_row"),
      A.numRows() + 1);

  KokkosSparse::Experimental::spgemm_symbolic(
      &kh, A.numRows(), B.numRows(), B.numCols(), A.graph.row_map,
      A.graph.entries, transposeA, B.graph.row_map, B.graph.entries, transposeB,
      row_mapC);

  entries_type entriesC;
  values_type valuesC;
  const size_t c_nnz_size = kh.get_spgemm_handle()->get_c_nnz();
  if (c_nnz_size) {
    entriesC = entries_type(
        Kokkos::view_alloc(Kokkos::WithoutInitializing, "entriesC"),
        c_nnz_size);
    valuesC =
        values_type(Kokkos::view_alloc(Kokkos::WithoutInitializing, "valuesC"),
                    c_nnz_size * blockDim * blockDim);
  }

  C = CMatrixType("C=AB", A.numRows(), B.numCols(), c_nnz_size, valuesC,
                  row_mapC, entriesC, blockDim);
}

template <class KernelHandle, class AMatrix, class BMatrix, class CMatrix>
void spgemm_numeric(KernelHandle& kh, const AMatrix& A, const bool Amode,
                    const BMatrix& B, const bool Bmode, CMatrix& C) {
  // using row_map_type = typename CMatrix::index_type::non_const_type;
  // using entries_type = typename CMatrix::row_map_type::non_const_type;
  // using values_type  = typename CMatrix::values_type::non_const_type;

  KokkosSparse::Experimental::spgemm_numeric(
      &kh, A.numRows(), B.numRows(), B.numCols(), A.graph.row_map,
      A.graph.entries, A.values, Amode, B.graph.row_map, B.graph.entries,
      B.values, Bmode, C.graph.row_map, C.graph.entries, C.values);
}

template <class KernelHandle, class AMatrix, class BMatrix, class CMatrix>
void block_spgemm_numeric(KernelHandle& kh, const AMatrix& A, const bool Amode,
                          const BMatrix& B, const bool Bmode, CMatrix& C) {
  auto blockDim = A.blockDim();
  if (blockDim != B.blockDim() or blockDim != C.blockDim()) {
    throw std::invalid_argument(
        "Block SpGEMM must be called for matrices with the same block size");
  }

  KokkosSparse::Experimental::spgemm_numeric(
      &kh, A.numRows(), B.numRows(), B.numCols(), A.graph.row_map,
      A.graph.entries, A.values, Amode, B.graph.row_map, B.graph.entries,
      B.values, Bmode, C.graph.row_map, C.graph.entries, C.values, blockDim);
}

template <class CMatrix, class AMatrix, class BMatrix>
CMatrix spgemm(const AMatrix& A, const bool Amode, const BMatrix& B,
               const bool Bmode) {
  using device_t  = typename CMatrix::device_type;
  using scalar_t  = typename CMatrix::value_type;
  using ordinal_t = typename CMatrix::ordinal_type;
  using size_type = typename CMatrix::size_type;
  using KKH       = KokkosKernels::Experimental::KokkosKernelsHandle<
      size_type, ordinal_t, scalar_t, typename device_t::execution_space,
      typename device_t::memory_space, typename device_t::memory_space>;
  KKH kh;
  kh.create_spgemm_handle();
  CMatrix C;
  spgemm_symbolic(kh, A, Amode, B, Bmode, C);
  spgemm_numeric(kh, A, Amode, B, Bmode, C);
  kh.destroy_spgemm_handle();
  return C;
}

}  // namespace KokkosSparse

#endif