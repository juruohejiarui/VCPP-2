#include "commandlist.h"

const std::string commandString[] = {
    "none"             , 
    "pstr"             , 
    "plabel"           , 
    "c_pvar"           , "b_pvar"           , "i16_pvar"         , "u16_pvar"         , "i32_pvar"         , "u32_pvar"         , "i64_pvar"         , "u64_pvar"         , "f32_pvar"         , "f64_pvar"         , 
    "o_pvar"           , "gv0_pvar"         , "gv1_pvar"         , "gv2_pvar"         , "gv3_pvar"         , "gv4_pvar"         , 
    "c_pglo"           , "b_pglo"           , "i16_pglo"         , "u16_pglo"         , "i32_pglo"         , "u32_pglo"         , "i64_pglo"         , "u64_pglo"         , "f32_pglo"         , "f64_pglo"         , 
    "o_pglo"           , "gv0_pglo"         , "gv1_pglo"         , "gv2_pglo"         , "gv3_pglo"         , "gv4_pglo"         , 
    "c_pmem"           , "b_pmem"           , "i16_pmem"         , "u16_pmem"         , "i32_pmem"         , "u32_pmem"         , "i64_pmem"         , "u64_pmem"         , "f32_pmem"         , "f64_pmem"         , 
    "o_pmem"           , "gv0_pmem"         , "gv1_pmem"         , "gv2_pmem"         , "gv3_pmem"         , "gv4_pmem"         , 
    "c_parrmem"        , "b_parrmem"        , "i16_parrmem"      , "u16_parrmem"      , "i32_parrmem"      , "u32_parrmem"      , "i64_parrmem"      , "u64_parrmem"      , "f32_parrmem"      , "f64_parrmem"      , 
    "o_parrmem"        , "gv0_parrmem"      , "gv1_parrmem"      , "gv2_parrmem"      , "gv3_parrmem"      , "gv4_parrmem"      , 
    "c_push"           , "b_push"           , "i16_push"         , "u16_push"         , "i32_push"         , "u32_push"         , "i64_push"         , "u64_push"         , "f32_push"         , "f64_push"         , 
    "o_push"           , "gv0_push"         , "gv1_push"         , "gv2_push"         , "gv3_push"         , "gv4_push"         , 
    "c_setvar"         , "b_setvar"         , "i16_setvar"       , "u16_setvar"       , "i32_setvar"       , "u32_setvar"       , "i64_setvar"       , "u64_setvar"       , "f32_setvar"       , "f64_setvar"       , 
    "o_setvar"         , "gv0_setvar"       , "gv1_setvar"       , "gv2_setvar"       , "gv3_setvar"       , "gv4_setvar"       , 
    "c_setglo"         , "b_setglo"         , "i16_setglo"       , "u16_setglo"       , "i32_setglo"       , "u32_setglo"       , "i64_setglo"       , "u64_setglo"       , "f32_setglo"       , "f64_setglo"       , 
    "o_setglo"         , "gv0_setglo"       , "gv1_setglo"       , "gv2_setglo"       , "gv3_setglo"       , "gv4_setglo"       , 
    "c_setmem"         , "b_setmem"         , "i16_setmem"       , "u16_setmem"       , "i32_setmem"       , "u32_setmem"       , "i64_setmem"       , "u64_setmem"       , "f32_setmem"       , "f64_setmem"       , 
    "o_setmem"         , "gv0_setmem"       , "gv1_setmem"       , "gv2_setmem"       , "gv3_setmem"       , "gv4_setmem"       , 
    "c_setarrmem"      , "b_setarrmem"      , "i16_setarrmem"    , "u16_setarrmem"    , "i32_setarrmem"    , "u32_setarrmem"    , "i64_setarrmem"    , "u64_setarrmem"    , "f32_setarrmem"    , "f64_setarrmem"    , 
    "o_setarrmem"      , "gv0_setarrmem"    , "gv1_setarrmem"    , "gv2_setarrmem"    , "gv3_setarrmem"    , "gv4_setarrmem"    , 
    "c_pop"            , "b_pop"            , "i16_pop"          , "u16_pop"          , "i32_pop"          , "u32_pop"          , "i64_pop"          , "u64_pop"          , "f32_pop"          , "f64_pop"          , 
    "o_pop"            , "gv0_pop"          , "gv1_pop"          , "gv2_pop"          , "gv3_pop"          , "gv4_pop"          , 
    "c_cpy"            , "b_cpy"            , "i16_cpy"          , "u16_cpy"          , "i32_cpy"          , "u32_cpy"          , "i64_cpy"          , "u64_cpy"          , "f32_cpy"          , "f64_cpy"          , 
    "o_cpy"            , "gv0_cpy"          , "gv1_cpy"          , "gv2_cpy"          , "gv3_cpy"          , "gv4_cpy"          , 
    "c_pincvar"        , "c_pincglo"        , "c_pincmem"        , "c_pincarrmem"     , "c_pdecvar"        , "c_pdecglo"        , "c_pdecmem"        , "c_pdecarrmem"     , "c_sincvar"        , "c_sincglo"        , 
    "c_sincmem"        , "c_sincarrmem"     , "c_sdecvar"        , "c_sdecglo"        , "c_sdecmem"        , "c_sdecarrmem"     , "b_pincvar"        , "b_pincglo"        , "b_pincmem"        , "b_pincarrmem"     , 
    "b_pdecvar"        , "b_pdecglo"        , "b_pdecmem"        , "b_pdecarrmem"     , "b_sincvar"        , "b_sincglo"        , "b_sincmem"        , "b_sincarrmem"     , "b_sdecvar"        , "b_sdecglo"        , 
    "b_sdecmem"        , "b_sdecarrmem"     , "i16_pincvar"      , "i16_pincglo"      , "i16_pincmem"      , "i16_pincarrmem"   , "i16_pdecvar"      , "i16_pdecglo"      , "i16_pdecmem"      , "i16_pdecarrmem"   , 
    "i16_sincvar"      , "i16_sincglo"      , "i16_sincmem"      , "i16_sincarrmem"   , "i16_sdecvar"      , "i16_sdecglo"      , "i16_sdecmem"      , "i16_sdecarrmem"   , "u16_pincvar"      , "u16_pincglo"      , 
    "u16_pincmem"      , "u16_pincarrmem"   , "u16_pdecvar"      , "u16_pdecglo"      , "u16_pdecmem"      , "u16_pdecarrmem"   , "u16_sincvar"      , "u16_sincglo"      , "u16_sincmem"      , "u16_sincarrmem"   , 
    "u16_sdecvar"      , "u16_sdecglo"      , "u16_sdecmem"      , "u16_sdecarrmem"   , "i32_pincvar"      , "i32_pincglo"      , "i32_pincmem"      , "i32_pincarrmem"   , "i32_pdecvar"      , "i32_pdecglo"      , 
    "i32_pdecmem"      , "i32_pdecarrmem"   , "i32_sincvar"      , "i32_sincglo"      , "i32_sincmem"      , "i32_sincarrmem"   , "i32_sdecvar"      , "i32_sdecglo"      , "i32_sdecmem"      , "i32_sdecarrmem"   , 
    "u32_pincvar"      , "u32_pincglo"      , "u32_pincmem"      , "u32_pincarrmem"   , "u32_pdecvar"      , "u32_pdecglo"      , "u32_pdecmem"      , "u32_pdecarrmem"   , "u32_sincvar"      , "u32_sincglo"      , 
    "u32_sincmem"      , "u32_sincarrmem"   , "u32_sdecvar"      , "u32_sdecglo"      , "u32_sdecmem"      , "u32_sdecarrmem"   , "i64_pincvar"      , "i64_pincglo"      , "i64_pincmem"      , "i64_pincarrmem"   , 
    "i64_pdecvar"      , "i64_pdecglo"      , "i64_pdecmem"      , "i64_pdecarrmem"   , "i64_sincvar"      , "i64_sincglo"      , "i64_sincmem"      , "i64_sincarrmem"   , "i64_sdecvar"      , "i64_sdecglo"      , 
    "i64_sdecmem"      , "i64_sdecarrmem"   , "u64_pincvar"      , "u64_pincglo"      , "u64_pincmem"      , "u64_pincarrmem"   , "u64_pdecvar"      , "u64_pdecglo"      , "u64_pdecmem"      , "u64_pdecarrmem"   , 
    "u64_sincvar"      , "u64_sincglo"      , "u64_sincmem"      , "u64_sincarrmem"   , "u64_sdecvar"      , "u64_sdecglo"      , "u64_sdecmem"      , "u64_sdecarrmem"   , 
    "c_addvar"         , "c_addglo"         , "c_addmem"         , "c_addarrmem"      , "c_subvar"         , "c_subglo"         , "c_submem"         , "c_subarrmem"      , "c_mulvar"         , "c_mulglo"         , 
    "c_mulmem"         , "c_mularrmem"      , "c_divvar"         , "c_divglo"         , "c_divmem"         , "c_divarrmem"      , "c_modvar"         , "c_modglo"         , "c_modmem"         , "c_modarrmem"      , 
    "c_andvar"         , "c_andglo"         , "c_andmem"         , "c_andarrmem"      , "c_orvar"          , "c_orglo"          , "c_ormem"          , "c_orarrmem"       , "c_xorvar"         , "c_xorglo"         , 
    "c_xormem"         , "c_xorarrmem"      , "c_shlvar"         , "c_shlglo"         , "c_shlmem"         , "c_shlarrmem"      , "c_shrvar"         , "c_shrglo"         , "c_shrmem"         , "c_shrarrmem"      , 
    "b_addvar"         , "b_addglo"         , "b_addmem"         , "b_addarrmem"      , "b_subvar"         , "b_subglo"         , "b_submem"         , "b_subarrmem"      , "b_mulvar"         , "b_mulglo"         , 
    "b_mulmem"         , "b_mularrmem"      , "b_divvar"         , "b_divglo"         , "b_divmem"         , "b_divarrmem"      , "b_modvar"         , "b_modglo"         , "b_modmem"         , "b_modarrmem"      , 
    "b_andvar"         , "b_andglo"         , "b_andmem"         , "b_andarrmem"      , "b_orvar"          , "b_orglo"          , "b_ormem"          , "b_orarrmem"       , "b_xorvar"         , "b_xorglo"         , 
    "b_xormem"         , "b_xorarrmem"      , "b_shlvar"         , "b_shlglo"         , "b_shlmem"         , "b_shlarrmem"      , "b_shrvar"         , "b_shrglo"         , "b_shrmem"         , "b_shrarrmem"      , 
    "i16_addvar"       , "i16_addglo"       , "i16_addmem"       , "i16_addarrmem"    , "i16_subvar"       , "i16_subglo"       , "i16_submem"       , "i16_subarrmem"    , "i16_mulvar"       , "i16_mulglo"       , 
    "i16_mulmem"       , "i16_mularrmem"    , "i16_divvar"       , "i16_divglo"       , "i16_divmem"       , "i16_divarrmem"    , "i16_modvar"       , "i16_modglo"       , "i16_modmem"       , "i16_modarrmem"    , 
    "i16_andvar"       , "i16_andglo"       , "i16_andmem"       , "i16_andarrmem"    , "i16_orvar"        , "i16_orglo"        , "i16_ormem"        , "i16_orarrmem"     , "i16_xorvar"       , "i16_xorglo"       , 
    "i16_xormem"       , "i16_xorarrmem"    , "i16_shlvar"       , "i16_shlglo"       , "i16_shlmem"       , "i16_shlarrmem"    , "i16_shrvar"       , "i16_shrglo"       , "i16_shrmem"       , "i16_shrarrmem"    , 
    "u16_addvar"       , "u16_addglo"       , "u16_addmem"       , "u16_addarrmem"    , "u16_subvar"       , "u16_subglo"       , "u16_submem"       , "u16_subarrmem"    , "u16_mulvar"       , "u16_mulglo"       , 
    "u16_mulmem"       , "u16_mularrmem"    , "u16_divvar"       , "u16_divglo"       , "u16_divmem"       , "u16_divarrmem"    , "u16_modvar"       , "u16_modglo"       , "u16_modmem"       , "u16_modarrmem"    , 
    "u16_andvar"       , "u16_andglo"       , "u16_andmem"       , "u16_andarrmem"    , "u16_orvar"        , "u16_orglo"        , "u16_ormem"        , "u16_orarrmem"     , "u16_xorvar"       , "u16_xorglo"       , 
    "u16_xormem"       , "u16_xorarrmem"    , "u16_shlvar"       , "u16_shlglo"       , "u16_shlmem"       , "u16_shlarrmem"    , "u16_shrvar"       , "u16_shrglo"       , "u16_shrmem"       , "u16_shrarrmem"    , 
    "i32_addvar"       , "i32_addglo"       , "i32_addmem"       , "i32_addarrmem"    , "i32_subvar"       , "i32_subglo"       , "i32_submem"       , "i32_subarrmem"    , "i32_mulvar"       , "i32_mulglo"       , 
    "i32_mulmem"       , "i32_mularrmem"    , "i32_divvar"       , "i32_divglo"       , "i32_divmem"       , "i32_divarrmem"    , "i32_modvar"       , "i32_modglo"       , "i32_modmem"       , "i32_modarrmem"    , 
    "i32_andvar"       , "i32_andglo"       , "i32_andmem"       , "i32_andarrmem"    , "i32_orvar"        , "i32_orglo"        , "i32_ormem"        , "i32_orarrmem"     , "i32_xorvar"       , "i32_xorglo"       , 
    "i32_xormem"       , "i32_xorarrmem"    , "i32_shlvar"       , "i32_shlglo"       , "i32_shlmem"       , "i32_shlarrmem"    , "i32_shrvar"       , "i32_shrglo"       , "i32_shrmem"       , "i32_shrarrmem"    , 
    "u32_addvar"       , "u32_addglo"       , "u32_addmem"       , "u32_addarrmem"    , "u32_subvar"       , "u32_subglo"       , "u32_submem"       , "u32_subarrmem"    , "u32_mulvar"       , "u32_mulglo"       , 
    "u32_mulmem"       , "u32_mularrmem"    , "u32_divvar"       , "u32_divglo"       , "u32_divmem"       , "u32_divarrmem"    , "u32_modvar"       , "u32_modglo"       , "u32_modmem"       , "u32_modarrmem"    , 
    "u32_andvar"       , "u32_andglo"       , "u32_andmem"       , "u32_andarrmem"    , "u32_orvar"        , "u32_orglo"        , "u32_ormem"        , "u32_orarrmem"     , "u32_xorvar"       , "u32_xorglo"       , 
    "u32_xormem"       , "u32_xorarrmem"    , "u32_shlvar"       , "u32_shlglo"       , "u32_shlmem"       , "u32_shlarrmem"    , "u32_shrvar"       , "u32_shrglo"       , "u32_shrmem"       , "u32_shrarrmem"    , 
    "i64_addvar"       , "i64_addglo"       , "i64_addmem"       , "i64_addarrmem"    , "i64_subvar"       , "i64_subglo"       , "i64_submem"       , "i64_subarrmem"    , "i64_mulvar"       , "i64_mulglo"       , 
    "i64_mulmem"       , "i64_mularrmem"    , "i64_divvar"       , "i64_divglo"       , "i64_divmem"       , "i64_divarrmem"    , "i64_modvar"       , "i64_modglo"       , "i64_modmem"       , "i64_modarrmem"    , 
    "i64_andvar"       , "i64_andglo"       , "i64_andmem"       , "i64_andarrmem"    , "i64_orvar"        , "i64_orglo"        , "i64_ormem"        , "i64_orarrmem"     , "i64_xorvar"       , "i64_xorglo"       , 
    "i64_xormem"       , "i64_xorarrmem"    , "i64_shlvar"       , "i64_shlglo"       , "i64_shlmem"       , "i64_shlarrmem"    , "i64_shrvar"       , "i64_shrglo"       , "i64_shrmem"       , "i64_shrarrmem"    , 
    "u64_addvar"       , "u64_addglo"       , "u64_addmem"       , "u64_addarrmem"    , "u64_subvar"       , "u64_subglo"       , "u64_submem"       , "u64_subarrmem"    , "u64_mulvar"       , "u64_mulglo"       , 
    "u64_mulmem"       , "u64_mularrmem"    , "u64_divvar"       , "u64_divglo"       , "u64_divmem"       , "u64_divarrmem"    , "u64_modvar"       , "u64_modglo"       , "u64_modmem"       , "u64_modarrmem"    , 
    "u64_andvar"       , "u64_andglo"       , "u64_andmem"       , "u64_andarrmem"    , "u64_orvar"        , "u64_orglo"        , "u64_ormem"        , "u64_orarrmem"     , "u64_xorvar"       , "u64_xorglo"       , 
    "u64_xormem"       , "u64_xorarrmem"    , "u64_shlvar"       , "u64_shlglo"       , "u64_shlmem"       , "u64_shlarrmem"    , "u64_shrvar"       , "u64_shrglo"       , "u64_shrmem"       , "u64_shrarrmem"    , 
    "f32_addvar"       , "f32_addglo"       , "f32_addmem"       , "f32_addarrmem"    , "f32_subvar"       , "f32_subglo"       , "f32_submem"       , "f32_subarrmem"    , "f32_mulvar"       , "f32_mulglo"       , 
    "f32_mulmem"       , "f32_mularrmem"    , "f32_divvar"       , "f32_divglo"       , "f32_divmem"       , "f32_divarrmem"    , "f32_modvar"       , "f32_modglo"       , "f32_modmem"       , "f32_modarrmem"    , 
    "f32_andvar"       , "f32_andglo"       , "f32_andmem"       , "f32_andarrmem"    , "f32_orvar"        , "f32_orglo"        , "f32_ormem"        , "f32_orarrmem"     , "f32_xorvar"       , "f32_xorglo"       , 
    "f32_xormem"       , "f32_xorarrmem"    , "f32_shlvar"       , "f32_shlglo"       , "f32_shlmem"       , "f32_shlarrmem"    , "f32_shrvar"       , "f32_shrglo"       , "f32_shrmem"       , "f32_shrarrmem"    , 
    "f64_addvar"       , "f64_addglo"       , "f64_addmem"       , "f64_addarrmem"    , "f64_subvar"       , "f64_subglo"       , "f64_submem"       , "f64_subarrmem"    , "f64_mulvar"       , "f64_mulglo"       , 
    "f64_mulmem"       , "f64_mularrmem"    , "f64_divvar"       , "f64_divglo"       , "f64_divmem"       , "f64_divarrmem"    , "f64_modvar"       , "f64_modglo"       , "f64_modmem"       , "f64_modarrmem"    , 
    "f64_andvar"       , "f64_andglo"       , "f64_andmem"       , "f64_andarrmem"    , "f64_orvar"        , "f64_orglo"        , "f64_ormem"        , "f64_orarrmem"     , "f64_xorvar"       , "f64_xorglo"       , 
    "f64_xormem"       , "f64_xorarrmem"    , "f64_shlvar"       , "f64_shlglo"       , "f64_shlmem"       , "f64_shlarrmem"    , "f64_shrvar"       , "f64_shrglo"       , "f64_shrmem"       , "f64_shrarrmem"    , 

    "c_add"            , "c_sub"            , "c_mul"            , "c_div"            , "b_add"            , "b_sub"            , "b_mul"            , "b_div"            , "i16_add"          , "i16_sub"          , 
    "i16_mul"          , "i16_div"          , "u16_add"          , "u16_sub"          , "u16_mul"          , "u16_div"          , "i32_add"          , "i32_sub"          , "i32_mul"          , "i32_div"          , 
    "u32_add"          , "u32_sub"          , "u32_mul"          , "u32_div"          , "i64_add"          , "i64_sub"          , "i64_mul"          , "i64_div"          , "u64_add"          , "u64_sub"          , 
    "u64_mul"          , "u64_div"          , "f32_add"          , "f32_sub"          , "f32_mul"          , "f32_div"          , "f64_add"          , "f64_sub"          , "f64_mul"          , "f64_div"          , 

    "c_mod"            , "c_and"            , "c_or"             , "c_xor"            , "c_shl"            , "c_shr"            , "c_not"            , "b_mod"            , "b_and"            , "b_or"             , 
    "b_xor"            , "b_shl"            , "b_shr"            , "b_not"            , "i16_mod"          , "i16_and"          , "i16_or"           , "i16_xor"          , "i16_shl"          , "i16_shr"          , 
    "i16_not"          , "u16_mod"          , "u16_and"          , "u16_or"           , "u16_xor"          , "u16_shl"          , "u16_shr"          , "u16_not"          , "i32_mod"          , "i32_and"          , 
    "i32_or"           , "i32_xor"          , "i32_shl"          , "i32_shr"          , "i32_not"          , "u32_mod"          , "u32_and"          , "u32_or"           , "u32_xor"          , "u32_shl"          , 
    "u32_shr"          , "u32_not"          , "i64_mod"          , "i64_and"          , "i64_or"           , "i64_xor"          , "i64_shl"          , "i64_shr"          , "i64_not"          , "u64_mod"          , 
    "u64_and"          , "u64_or"           , "u64_xor"          , "u64_shl"          , "u64_shr"          , "u64_not"          , 
    "c_eq"             , "c_ne"             , "c_gt"             , "c_ge"             , "c_ls"             , "c_le"             , "b_eq"             , "b_ne"             , "b_gt"             , "b_ge"             , 
    "b_ls"             , "b_le"             , "i16_eq"           , "i16_ne"           , "i16_gt"           , "i16_ge"           , "i16_ls"           , "i16_le"           , "u16_eq"           , "u16_ne"           , 
    "u16_gt"           , "u16_ge"           , "u16_ls"           , "u16_le"           , "i32_eq"           , "i32_ne"           , "i32_gt"           , "i32_ge"           , "i32_ls"           , "i32_le"           , 
    "u32_eq"           , "u32_ne"           , "u32_gt"           , "u32_ge"           , "u32_ls"           , "u32_le"           , "i64_eq"           , "i64_ne"           , "i64_gt"           , "i64_ge"           , 
    "i64_ls"           , "i64_le"           , "u64_eq"           , "u64_ne"           , "u64_gt"           , "u64_ge"           , "u64_ls"           , "u64_le"           , "f32_eq"           , "f32_ne"           , 
    "f32_gt"           , "f32_ge"           , "f32_ls"           , "f32_le"           , "f64_eq"           , "f64_ne"           , "f64_gt"           , "f64_ge"           , "f64_ls"           , "f64_le"           , 
    "o_eq"             , "o_ne"             , "o_gt"             , "o_ge"             , "o_ls"             , "o_le"             , 
    "c_c_cvt"          , "c_b_cvt"          , "c_i16_cvt"        , "c_u16_cvt"        , "c_i32_cvt"        , "c_u32_cvt"        , "c_i64_cvt"        , "c_u64_cvt"        , "c_f32_cvt"        , "c_f64_cvt"        , 
    "c_o_cvt"          , "b_c_cvt"          , "b_b_cvt"          , "b_i16_cvt"        , "b_u16_cvt"        , "b_i32_cvt"        , "b_u32_cvt"        , "b_i64_cvt"        , "b_u64_cvt"        , "b_f32_cvt"        , 
    "b_f64_cvt"        , "b_o_cvt"          , "i16_c_cvt"        , "i16_b_cvt"        , "i16_i16_cvt"      , "i16_u16_cvt"      , "i16_i32_cvt"      , "i16_u32_cvt"      , "i16_i64_cvt"      , "i16_u64_cvt"      , 
    "i16_f32_cvt"      , "i16_f64_cvt"      , "i16_o_cvt"        , "u16_c_cvt"        , "u16_b_cvt"        , "u16_i16_cvt"      , "u16_u16_cvt"      , "u16_i32_cvt"      , "u16_u32_cvt"      , "u16_i64_cvt"      , 
    "u16_u64_cvt"      , "u16_f32_cvt"      , "u16_f64_cvt"      , "u16_o_cvt"        , "i32_c_cvt"        , "i32_b_cvt"        , "i32_i16_cvt"      , "i32_u16_cvt"      , "i32_i32_cvt"      , "i32_u32_cvt"      , 
    "i32_i64_cvt"      , "i32_u64_cvt"      , "i32_f32_cvt"      , "i32_f64_cvt"      , "i32_o_cvt"        , "u32_c_cvt"        , "u32_b_cvt"        , "u32_i16_cvt"      , "u32_u16_cvt"      , "u32_i32_cvt"      , 
    "u32_u32_cvt"      , "u32_i64_cvt"      , "u32_u64_cvt"      , "u32_f32_cvt"      , "u32_f64_cvt"      , "u32_o_cvt"        , "i64_c_cvt"        , "i64_b_cvt"        , "i64_i16_cvt"      , "i64_u16_cvt"      , 
    "i64_i32_cvt"      , "i64_u32_cvt"      , "i64_i64_cvt"      , "i64_u64_cvt"      , "i64_f32_cvt"      , "i64_f64_cvt"      , "i64_o_cvt"        , "u64_c_cvt"        , "u64_b_cvt"        , "u64_i16_cvt"      , 
    "u64_u16_cvt"      , "u64_i32_cvt"      , "u64_u32_cvt"      , "u64_i64_cvt"      , "u64_u64_cvt"      , "u64_f32_cvt"      , "u64_f64_cvt"      , "u64_o_cvt"        , "f32_c_cvt"        , "f32_b_cvt"        , 
    "f32_i16_cvt"      , "f32_u16_cvt"      , "f32_i32_cvt"      , "f32_u32_cvt"      , "f32_i64_cvt"      , "f32_u64_cvt"      , "f32_f32_cvt"      , "f32_f64_cvt"      , "f32_o_cvt"        , "f64_c_cvt"        , 
    "f64_b_cvt"        , "f64_i16_cvt"      , "f64_u16_cvt"      , "f64_i32_cvt"      , "f64_u32_cvt"      , "f64_i64_cvt"      , "f64_u64_cvt"      , "f64_f32_cvt"      , "f64_f64_cvt"      , "f64_o_cvt"        , 
    "o_c_cvt"          , "o_b_cvt"          , "o_i16_cvt"        , "o_u16_cvt"        , "o_i32_cvt"        , "o_u32_cvt"        , "o_i64_cvt"        , "o_u64_cvt"        , "o_f32_cvt"        , "o_f64_cvt"        , 
    "o_o_cvt"          , 
    "newobj"           , 
    "newarr"           , 
    "call"             , 
    "vcall"            , 
    "ret"              , 
    "vret"             , 
    "setlocal"         , 
    "jmp"              , 
    "jp"               , 
    "jz"               , 
    "setarg"           , 
    "getarg"           , 
    "setgtbl"          , 
    "setclgtbl"        , 
    "getgtbl"          , 
    "getgtblsz"        , 
    "sys"              , 
    "setflag"          , 
    "pushflag"         , 
    "setpause"         ,
    "unknown"
};

const std::string pretreatCommandString[] {
    "RELY", "EXTERN", "EXPOSE", "LABEL", "GLOMEM", "HINT", "STRING", "DEF", "TYPEDATA_BEGIN", "TYPEDATA_END", "unknown"
};

const int tCommandNumber = 108;
const std::string tCommandString[] = {
    "none"    , "pstr"    , "plabel"  , "pvar"    , "pglo"    , "pmem"    , "parrmem" , "push"    , "setvar"  , "setglo"  , 
    "setmem"  , "setarrmem", "pop"     , "cpy"     , "pincvar" , "pincglo" , "pincmem" , "pincarrmem", "pdecvar" , "pdecglo" , 
    "pdecmem" , "pdecarrmem", "sincvar" , "sincglo" , "sincmem" , "sincarrmem", "sdecvar" , "sdecglo" , "sdecmem" , "sdecarrmem", 
    "addvar"  , "addglo"  , "addmem"  , "addarrmem", "subvar"  , "subglo"  , "submem"  , "subarrmem", "mulvar"  , "mulglo"  , 
    "mulmem"  , "mularrmem", "divvar"  , "divglo"  , "divmem"  , "divarrmem", "modvar"  , "modglo"  , "modmem"  , "modarrmem", 
    "andvar"  , "andglo"  , "andmem"  , "andarrmem", "orvar"   , "orglo"   , "ormem"   , "orarrmem", "xorvar"  , "xorglo"  , 
    "xormem"  , "xorarrmem", "shlvar"  , "shlglo"  , "shlmem"  , "shlarrmem", "shrvar"  , "shrglo"  , "shrmem"  , "shrarrmem", 
    "add"     , "sub"     , "mul"     , "div"     , "mod"     , "and"     , "or"      , "xor"     , "shl"     , "shr"     , 
    "not"     , "eq"      , "ne"      , "gt"      , "ge"      , "ls"      , "le"      , "cvt"     , "newobj"  , "newarr"  , 
    "call"    , "vcall"   , "ret"     , "vret"    , "setlocal", "jmp"     , "jp"      , "jz"      , "setarg"  , "getarg"  , 
    "setgtbl" , "setclgtbl", "getgtbl" , "getgtblsz", "sys"     , "setflag" , "pushflag", "setpause", 
    "unknown"
};

const int commandNumber = 1010, pretreatCommandNumber = 10;

Command getCommand(const std::string &name) {
    for (int i = 0; i < commandNumber; i++) if (name == commandString[i]) return (Command)i;
    return (Command)commandNumber;
}

TCommand getTCommand(const std::string &name) {
    for (int i = 0; i < tCommandNumber; i++) if (name == tCommandString[i]) return (TCommand)i;
    return (TCommand)commandNumber;
}
PretreatCommand getPretreatCommand(const std::string &name) {
    for (int i = 0; i < pretreatCommandNumber; i++) if (name == pretreatCommandString[i]) return (PretreatCommand)i;
    return (PretreatCommand)pretreatCommandNumber;
}

Command wrap(TCommand tcmd, DataTypeModifier dtMdf) {
    return getCommand(dataTypeModifierStr[(int)dtMdf] + "_" + tCommandString[(int)tcmd]);
}
Command wrap(TCommand tcmd,  DataTypeModifier dtMdf1, DataTypeModifier dtMdf2) {
    return getCommand(dataTypeModifierStr[(int)dtMdf1] + "_" + dataTypeModifierStr[(int)dtMdf1] + "_" + tCommandString[(int)tcmd]);
}