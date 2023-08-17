;; Includes:
;; - 7.4. Vector Unit-Stride Instructions
;; - todo: 11.15 Vector Integer Merge Instructions
;; - todo: 11.16 Vector Integer Move Instructions
;; - todo: 13.16 Vector Floating-Point Move Instruction
;; - todo: 15.1 Vector Mask-Register Logical Instructions
;; -------------------------------------------------------------------------------

;; vle.v/vlb.v/vlh.v/vlw.v
;; todo: vlbu.v/vlhu.v/vlwu.v
;; vse.v/vsb.v/vsh.v/vsw.v
;; todo: vmv.v.v/vmv.v.x/vmv.v.i/vfmv.v.f.
;; For vle.v/vmv.v.v/vmv.v.x/vmv.v.i/vfmv.v.f, we may need merge and mask operand.
;; For vse.v, we don't need merge operand, so it should always match "vu".
;; constraint alternative 0 ~ 1 match vle.v.
;; constraint alternative 2 match vse.v.
;; constraint alternative 3 match vmv.v.v.
;; constraint alternative 4 match vmv.v.i.
;; For vmv.v.i, we allow 2 following cases:
;;    1. (const_vector:RVVMF8QI repeat [
;;                (const_int:QI N)]), -15 <= N < 16.
;;    2. (const_vector:RVVMF2SF repeat [
;;                (const_double:SF 0.0 [0x0.0p+0])]).

;; We add "MEM_P (operands[0]) || MEM_P (operands[3]) || CONST_VECTOR_P (operands[1])" here to
;; make sure we don't want CSE to generate the following pattern:
;; (insn 17 8 19 2 (set (reg:RVVMF4HI 134 [ _1 ])
;;       (if_then_else:RVVMF4HI (unspec:RVVM1BI [
;;                   (reg/v:RVVM1BI 137 [ mask ])
;;                   (reg:DI 151)
;;                   (const_int 0 [0]) repeated x3
;;                   (reg:SI 66 vl)
;;                   (reg:SI 67 vtype)
;;               ] UNSPEC_VPREDICATE)
;;           (const_vector:RVVMF4HI repeat [
;;                   (const_int 0 [0])
;;               ])
;;           (reg/v:RVVMF4HI 140 [ merge ]))) "rvv.c":8:12 608 {pred_movvnx1hi}
;;    (expr_list:REG_DEAD (reg:DI 151)
;;       (expr_list:REG_DEAD (reg/v:RVVMF4HI 140 [ merge ])
;;           (expr_list:REG_DEAD (reg/v:RVVM1BI 137 [ mask ])
;;               (nil)))))
;; Since both vmv.v.v and vmv.v.i doesn't have mask operand.
(define_insn_and_split "@pred_mov<mode>"
  [(set (match_operand:V_VLS 0 "nonimmediate_operand"  "=vr,    vr,    vd,     m,    vr,    vr,    vr,    vr")
    (if_then_else:V_VLS
      (unspec:<VM>
        [(match_operand:<VM> 1 "vector_mask_operand" "vmWc1,   Wc1,    vm, vmWc1,   Wc1,   Wc1,   Wc1,   Wc1")
         (match_operand 4 "vector_length_operand"    "   rK,    rK,    rK,    rK,    rK,    rK,    rK,    rK")
         (match_operand 5 "const_int_operand"        "    i,     i,     i,     i,     i,     i,     i,     i")
         (match_operand 6 "const_int_operand"        "    i,     i,     i,     i,     i,     i,     i,     i")
         (match_operand 7 "const_int_operand"        "    i,     i,     i,     i,     i,     i,     i,     i")
         (reg:SI VL_REGNUM)
         (reg:SI VTYPE_REGNUM)] UNSPEC_VPREDICATE)
      (match_operand:V_VLS 3 "vector_move_operand"   "    m,     m,     m,    vr,    vr,    vr, viWc0, viWc0")
      (match_operand:V_VLS 2 "vector_merge_operand"  "    0,    vu,    vu,    vu,    vu,     0,    vu,     0")))]
  "TARGET_VECTOR && (MEM_P (operands[0]) || MEM_P (operands[3])
   || CONST_VECTOR_P (operands[1]))"
  "@
   vle<sew>.v\t%0,%3%p1
   vle<sew>.v\t%0,%3
   vle<sew>.v\t%0,%3,%1.t
   vse<sew>.v\t%3,%0%p1
   vmv.v.v\t%0,%3
   vmv.v.v\t%0,%3
   vmv.v.i\t%0,%v3
   vmv.v.i\t%0,%v3"
  "&& register_operand (operands[0], <MODE>mode)
   && register_operand (operands[3], <MODE>mode)
   && satisfies_constraint_vu (operands[2])
   && INTVAL (operands[7]) == riscv_vector::VLMAX"
  [(set (match_dup 0) (match_dup 3))]
  ""
  [(set_attr "type" "vlde,vlde,vlde,vste,vimov,vimov,vimov,vimov")
   (set_attr "mode" "<MODE>")])

(define_insn "load2")
[(set (match_operand:QI 0 "nonimmediate_operand")
      (load:QI (match_operand:QI 1 "vr")
                (match_operand:QI 2 "r")))]
"TARGET_XTHREAD"
"vlb.v\t%0, %1, %2"
[(set_attr "type" "vlde")]


;; Dedicated pattern for vse.v instruction since we can't reuse pred_mov pattern to include
;; memory operand as input which will produce inferior codegen.
(define_insn "@pred_store<mode>"
  [(set (match_operand:V 0 "memory_operand"                 "+m")
	(if_then_else:V
	  (unspec:<VM>
	    [(match_operand:<VM> 1 "vector_mask_operand" "vmWc1")
	     (match_operand 3 "vector_length_operand"    "   rK")
	     (match_operand 4 "const_int_operand"        "    i")
	     (reg:SI VL_REGNUM)
	     (reg:SI VTYPE_REGNUM)] UNSPEC_VPREDICATE)
	  (match_operand:V 2 "register_operand"         "    vr")
	  (match_dup 0)))]
  "TARGET_VECTOR"
  "vse<sew>.v\t%2,%0%p1"
  [(set_attr "type" "vste")
   (set_attr "mode" "<MODE>")
   (set (attr "avl_type") (symbol_ref "INTVAL (operands[4])"))
   (set_attr "vl_op_idx" "3")])

;; =========================================================================
;; == Quading Ternary arithmetic
;; =========================================================================

;; -------------------------------------------------------------------------
;; ---- [INT] VQMACC
;; -------------------------------------------------------------------------
;; Includes:
;; - vqmacc.vv
;; - vqmaccu.vv
;; -------------------------------------------------------------------------

;; Combine ext + ext + fma ===> widen fma.
;; Most of circumstantces, LoopVectorizer will generate the following IR:
;;   vect__8.64_40 = (vector([4,4]) int) vect__7.63_41;
;;   vect__11.68_35 = (vector([4,4]) int) vect__10.67_36;
;;   vect__13.70_33 = .FMA (vect__11.68_35, vect__8.64_40, vect__4.60_45);
(define_insn_and_split "*<optab>_fma<mode>"
  [(set (match_operand:VWEXTI 0 "register_operand")
	(plus:VWEXTI
	  (mult:VWEXTI
	    (any_extend:VWEXTI
	      (match_operand:<V_DOUBLE_TRUNC> 2 "register_operand"))
	    (any_extend:VWEXTI
	      (match_operand:<V_DOUBLE_TRUNC> 3 "register_operand")))
	  (match_operand:VWEXTI 1 "register_operand")))]
  "TARGET_VECTOR && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
  {
    riscv_vector::emit_vlmax_ternary_insn (code_for_pred_widen_mul_plus (<CODE>, <MODE>mode),
					   riscv_vector::RVV_WIDEN_TERNOP, operands);
    DONE;
  }
  [(set_attr "type" "viwmuladd")
   (set_attr "mode" "<V_DOUBLE_TRUNC>")])

;; This helps to match ext + fma.
(define_insn_and_split "*single_<optab>mult_plus<mode>"
  [(set (match_operand:VWEXTI 0 "register_operand")
	(plus:VWEXTI
	  (mult:VWEXTI
	    (any_extend:VWEXTI
	      (match_operand:<V_DOUBLE_TRUNC> 2 "register_operand"))
	    (match_operand:VWEXTI 3 "register_operand"))
	  (match_operand:VWEXTI 1 "register_operand")))]
  "TARGET_VECTOR && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
  {
    insn_code icode = code_for_pred_vf2 (<CODE>, <MODE>mode);
    rtx tmp = gen_reg_rtx (<MODE>mode);
    rtx ext_ops[] = {tmp, operands[2]};
    riscv_vector::emit_vlmax_insn (icode, riscv_vector::RVV_UNOP, ext_ops);

    rtx dst = expand_ternary_op (<MODE>mode, fma_optab, tmp, operands[3],
				 operands[1], operands[0], 0);
    emit_move_insn (operands[0], dst);
    DONE;
  }
  [(set_attr "type" "viwmuladd")
   (set_attr "mode" "<V_DOUBLE_TRUNC>")])

;; Combine ext + ext + mult + plus ===> widen fma.
;; We have some special cases generated by LoopVectorizer:
;;   vect__8.18_46 = (vector([8,8]) signed short) vect__7.17_47;
;;   vect__11.22_41 = (vector([8,8]) signed short) vect__10.21_42;
;;   vect__12.23_40 = vect__11.22_41 * vect__8.18_46;
;;   vect__14.25_38 = vect__13.24_39 + vect__5.14_51;
;; This situation doesn't generate FMA IR.
(define_insn_and_split "*double_<optab>mult_plus<mode>"
  [(set (match_operand:VWEXTI 0 "register_operand")
	(if_then_else:VWEXTI
	  (unspec:<VM>
	    [(match_operand:<VM> 1 "vector_mask_operand")
	     (match_operand 6 "vector_length_operand")
	     (match_operand 7 "const_int_operand")
	     (match_operand 8 "const_int_operand")
	     (match_operand 9 "const_int_operand")
	     (reg:SI VL_REGNUM)
	     (reg:SI VTYPE_REGNUM)] UNSPEC_VPREDICATE)
          (plus:VWEXTI
	    (if_then_else:VWEXTI
	      (unspec:<VM>
	        [(match_dup 1)
	         (match_dup 6)
	         (match_dup 7)
	         (match_dup 8)
	         (match_dup 9)
	         (reg:SI VL_REGNUM)
	         (reg:SI VTYPE_REGNUM)] UNSPEC_VPREDICATE)
	      (mult:VWEXTI
	        (any_extend:VWEXTI
	          (match_operand:<V_DOUBLE_TRUNC> 4 "register_operand"))
	        (any_extend:VWEXTI
	          (match_operand:<V_DOUBLE_TRUNC> 5 "register_operand")))
              (match_operand:VWEXTI 2 "vector_undef_operand"))
	    (match_operand:VWEXTI 3 "register_operand"))
          (match_dup 2)))]
  "TARGET_VECTOR && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
  {
    emit_insn (gen_pred_widen_mul_plus (<CODE>, <MODE>mode, operands[0],
					operands[1], operands[3], operands[4],
					operands[5], operands[6], operands[7],
					operands[8], operands[9]));
    DONE;
  }
  [(set_attr "type" "viwmuladd")
   (set_attr "mode" "<V_DOUBLE_TRUNC>")])

;; Combine sign_extend + zero_extend + fma ===> widen fma (su).
(define_insn_and_split "*sign_zero_extend_fma"
  [(set (match_operand:VWEXTI 0 "register_operand")
	(plus:VWEXTI
	  (mult:VWEXTI
	    (sign_extend:VWEXTI
	      (match_operand:<V_DOUBLE_TRUNC> 2 "register_operand"))
	    (zero_extend:VWEXTI
	      (match_operand:<V_DOUBLE_TRUNC> 3 "register_operand")))
	  (match_operand:VWEXTI 1 "register_operand")))]
  "TARGET_VECTOR && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
  {
    riscv_vector::emit_vlmax_ternary_insn (code_for_pred_widen_mul_plussu (<MODE>mode),
					   riscv_vector::RVV_WIDEN_TERNOP, operands);
    DONE;
  }
  [(set_attr "type" "viwmuladd")
   (set_attr "mode" "<V_DOUBLE_TRUNC>")])

;; This helps to match zero_extend + sign_extend + fma.
(define_insn_and_split "*zero_sign_extend_fma"
  [(set (match_operand:VWEXTI 0 "register_operand")
	(plus:VWEXTI
	  (mult:VWEXTI
	    (zero_extend:VWEXTI
	      (match_operand:<V_DOUBLE_TRUNC> 2 "register_operand"))
	    (sign_extend:VWEXTI
	      (match_operand:<V_DOUBLE_TRUNC> 3 "register_operand")))
	  (match_operand:VWEXTI 1 "register_operand")))]
  "TARGET_VECTOR && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
  {
    rtx ops[] = {operands[0], operands[1], operands[3], operands[2]};
    riscv_vector::emit_vlmax_ternary_insn (code_for_pred_widen_mul_plussu (<MODE>mode),
					   riscv_vector::RVV_WIDEN_TERNOP, ops);
    DONE;
  }
  [(set_attr "type" "viwmuladd")
   (set_attr "mode" "<V_DOUBLE_TRUNC>")])
