! Solves a system of nonlinear equation with the trust region method
!
! Implementation heavily inspired from the hybrj function from MINPACK

! Copyright © 2019-2021 Dynare Team
!
! This file is part of Dynare.
!
! Dynare is free software: you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation, either version 3 of the License, or
! (at your option) any later version.
!
! Dynare is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! GNU General Public License for more details.
!
! You should have received a copy of the GNU General Public License
! along with Dynare.  If not, see <https://www.gnu.org/licenses/>.

module trust_region
  use iso_fortran_env
  use lapack
  implicit none

  private
  public :: trust_region_solve

contains
  subroutine trust_region_solve(x, f, info, tolx, tolf, maxiter, factor)

    real(real64), dimension(:), intent(inout) :: x ! On entry: guess value; on exit: solution
    interface
       subroutine f(x1, fvec, fjac)
         use iso_fortran_env
         real(real64), dimension(:), intent(in) :: x1
         real(real64), dimension(size(x)), intent(out) :: fvec
         real(real64), dimension(size(x),size(x)), intent(out), optional :: fjac
       end subroutine f
    end interface
    ! Exit code:
    !  1 = success (relative error between two consecutive iterates is at most tolx)
    !  2 = maximum number of iterations reached
    !  3 = tolx is too small, no further improvement of the approximate solution x is possible
    !  4 = iteration is not making good progress, as measured by the improvement from the last maxslowiter iterations
    integer, intent(out) :: info
    real(real64), intent(in), optional :: tolx, tolf ! Tolerances in x and f
    integer, intent(in), optional :: maxiter ! Maximum number of iterations
    real(real64), intent(in), optional :: factor ! Used in determining the initial step bound.

    real(real64) :: tolx_actual, tolf_actual
    integer :: maxiter_actual
    real(real64) :: factor_actual
    integer, parameter :: maxslowiter = 15 ! Maximum number of consecutive iterations with slow progress
    real(real64), parameter :: macheps = epsilon(x)
    real(real64) :: delta ! Radius of the trust region
    real(real64), dimension(size(x)) :: fvec ! Current function value
    real(real64) :: fn ! Norm of the current function value
    real(real64), dimension(size(x)) :: jcn, dg ! Jacobian column-norms and rescaling factors
    real(real64), dimension(size(x), size(x)) :: fjac ! Jacobian matrix
    real(real64), dimension(size(x)) :: gn ! Gauss-Newton direction
    logical :: recompute_gn ! Whether to recompute Gauss-Newton direction at next dogleg
    integer :: niter ! Current iteration
    integer :: ncsucc ! Number of consecutive successful iterations
    integer :: ncslow ! Number of consecutive iterations with slow progress

    ! Initialize variables associated to optional arguments
    if (present(tolx)) then
       tolx_actual = tolx
    else
       tolx_actual = 1e-6_real64
    end if
    if (present(tolf)) then
       tolf_actual = tolf
    else
       tolf_actual = 1e-6_real64
    end if
    if (present(maxiter)) then
       maxiter_actual = maxiter
    else
       maxiter_actual = 50
    end if
    if (present(factor)) then
       factor_actual = factor
    else
       factor_actual = 100.0_real64
    end if

    niter = 1
    ncsucc = 0
    ncslow = 0
    info = 0

    ! Initial function evaluation
    call f_and_update_norms

    do
       ! Compute scaling factors
       if (niter == 1) then
          where (jcn /= 0)
             dg = jcn
          elsewhere
             dg = 1
          end where
       else
          ! Rescale adaptatively
          ! MINPACK uses dg=max(dg, jcn), but this means that scale factors
          ! will never decrease, which can be bad if the Jacobian at initial
          ! guess has large columns that later decrease
          dg = max(0.1_real64 * dg, jcn)
       end if

       block
         ! Declare variables that are not kept across iterations
         real(real64) :: xnorm ! Norm of rescaled x
         real(real64), dimension(size(x)) :: p ! Candidate increment computed by dogleg
         real(real64) :: pnorm ! Norm of rescaled p
         real(real64), dimension(size(x)) :: x2 ! Candidate x for next iteration
         real(real64), dimension(size(x)) :: fvec2 ! Candidate function values
         real(real64) :: fn2 ! Norm of the candidate function values
         real(real64), dimension(size(x)) :: w ! Candidate in the approximated linear model
         real(real64) :: actred, prered, ratio ! Actual and predicted reduction, and their ratio

         xnorm = norm2(dg * x)

         if (niter == 1) then
            if (xnorm > 0) then
               delta = xnorm
            else
               delta = 1
            end if
            delta = delta*factor
         end if

         ! Get trust-region model (dogleg) minimizer
         call dogleg(fjac, fvec, dg, delta, p, gn, recompute_gn)
         recompute_gn = .false.
         p = -p
         pnorm = norm2(dg * p)

         ! Compute candidate point x2 and function value there
         x2 = x + p
         call f(x2, fvec2)
         fn2 = norm2(fvec2)

         ! Actual reduction
         if (fn2 < fn) then
            actred = 1 - (fn2/fn)**2
         else
            actred = -1
         end if

         ! Predicted reduction
         ! Could be replaced by t => norm2(fvec + matmul(fjac, p))
         ! First, compute w = fvec + fjac·p
         associate (n => int(size(x), blint))
           w = fvec
           call dgemv("N", n, n, 1._real64, fjac, n, p, 1_blint, 1._real64, w, 1_blint)
         end associate
         associate (t => norm2(w))
           if (t < fn) then
              prered = 1 - (t/fn)**2
           else
              prered = 0
           end if
         end associate

         ! Ratio
         if (prered > 0) then
            ratio = actred / prered
         else
            ratio = 0
         end if

         ! Update delta
         if (niter == 1) delta = min(delta, pnorm) ! On 1st iteration, adjust radius
         if (ratio < 0.1_real64) then ! Failed iteration
            delta = delta / 2
            ncsucc = 0
         else ! Successful iteration
            ncsucc = ncsucc + 1
            if (abs(1-ratio) <= 0.1_real64) then
               delta = 2*pnorm
            else if (ratio >= 0.5_real64 .or. ncsucc > 1) then
               delta = max(delta, 2*pnorm)
            end if
         end if

         ! If successful iteration, move x and update various variables
         if (ratio >= 1e-4_real64) then
            x = x2
            call f_and_update_norms
            xnorm = norm2(dg * x)
         end if

         ! Determine progress of the iteration
         ncslow = ncslow + 1
         if (actred >= 0.001_real64) ncslow = 0

         ! Increment iteration counter and exit if maximum reached
         niter = niter + 1
         if (niter == maxiter_actual) info = 2

         ! Test for convergence
         if (delta <= tolx_actual*xnorm .or. fn <= tolf_actual) info = 1

         ! Tests for termination and stringent tolerances
         if (max(0.1_real64*delta, pnorm) <= 10*epsilon(xnorm)*xnorm) info = 3
         if (ncslow == maxslowiter) info = 4
       end block

       if (info /= 0) exit
    end do
  contains
    ! Given x, updates fvec, fjac, fn and jcn
    subroutine f_and_update_norms
      integer :: i

      call f(x, fvec, fjac)
      recompute_gn = .true.
      fn = norm2(fvec)
      do i = 1, size(jcn)
         jcn(i) = norm2(fjac(:, i))
      end do
    end subroutine f_and_update_norms
  end subroutine trust_region_solve

  ! Solve the double dogleg trust-region least-squares problem:
  ! Minimize ‖r·x−b‖₂ subject to the constraint ‖d.*x‖ ≤ delta (where “.*”
  ! designates element-by-element multiplication),
  ! x is a convex combination of the Gauss-Newton and scaled gradient
  subroutine dogleg(r, b, d, delta, x, gn, recompute_gn)
    ! The arrays used in BLAS/LAPACK calls are required to be contiguous, to
    ! avoid temporary copies before calling BLAS/LAPACK.
    real(real64), dimension(:), contiguous, intent(in) :: b
    real(real64), dimension(:), intent(in) :: d
    real(real64), dimension(:,:), contiguous, intent(in) :: r
    real(real64), intent(in) :: delta ! Radius of the trust region
    real(real64), dimension(:), intent(out) :: x ! Solution of the problem
    real(real64), dimension(:), contiguous, intent(inout) :: gn ! Gauss-Newton direction
    logical, intent(in) :: recompute_gn ! Whether to re-compute Gauss-Newton direction

    integer(blint) :: n

    n = size(x)
    if (size(b) /= n .or. size(d) /= n .or. size(r, 1) /= n .or. size(r, 2) /= n) &
         error stop "Inconsistent dimensions"

    ! Compute Gauss-Newton direction: gn = r⁻¹·b
    if (recompute_gn) then
       block
         real(real64), dimension(size(x), size(x)) :: r_plu
         integer(blint), dimension(size(x)) :: ipiv
         integer(blint) :: info
         gn = b
         r_plu = r
         call dgesv(n, 1_blint, r_plu, n, ipiv, gn, n, info)
         ! TODO: use same trick as NLSolve.jl in case of singularity
         if (info /= 0) error stop "LAPACK dgesv error"
       end block
    end if

    associate (xn => norm2(d*gn))
      if (xn <= delta) then
         x = gn
      else
         ! Gauss-Newton direction is too big, get scaled gradient
         block
           real(real64), dimension(size(x)) :: s, t
           real(real64) :: snm, alpha

           ! s = rᵀ·b ./ d
           ! Alternatively, could use: s = matmul(transpose(r), b) / d
           call dgemv("T", n, n, 1._real64, r, n, b, 1_blint, 0._real64, s, 1_blint)
           s = s / d
           associate (sn => norm2(s))
             if (sn > 0) then
                ! Normalize and rescale
                s = s / sn / d

                ! Get the line minimizer in s direction
                ! t = r·s
                ! Alternatively, could use tn => norm2(matmul(r, s))
                call dgemv("N", n, n, 1._real64, r, n, s, 1_blint, 0._real64, t, 1_blint)
                associate (tn => norm2(t))
                  snm = sn / tn**2
                  if (snm < delta) then
                     ! Get the dogleg path minimizer
                     associate (bn => norm2(b), dxn => delta/xn, snmd => snm/delta)
                       associate (tmp => (bn/sn) * (bn/xn) * snmd)
                         alpha = dxn*(1-snmd**2) / (tmp - dxn*snmd**2 + &
                              sqrt((tmp-dxn)**2 + (1-dxn**2)*(1-snmd**2)))
                       end associate
                     end associate
                  else
                     alpha = 0
                  end if
                end associate
             else
                alpha = delta / xn
                snm = 0
             end if
           end associate

           x = alpha * gn + ((1-alpha) * min(snm, delta)) * s
         end block
      end if
    end associate
  end subroutine dogleg
end module trust_region
