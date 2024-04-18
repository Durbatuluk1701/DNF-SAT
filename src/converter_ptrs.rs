use core::panic;
use std::{
    borrow::BorrowMut,
    env,
    fs::File,
    io::{BufRead, BufReader},
    sync::{atomic::AtomicBool, Arc, Mutex},
};

#[derive(Hash, Eq, PartialEq, Debug, Clone)]
enum Formula {
    FVar(u32),
    FNeg(Box<Formula>),
    FDisj(Vec<Formula>),
    FConj(Vec<Formula>),
}
use crossbeam::atomic::AtomicCell;
use rayon::{
    current_thread_index,
    iter::{
        IntoParallelIterator, IntoParallelRefIterator, IntoParallelRefMutIterator, ParallelBridge,
        ParallelIterator,
    },
};
use Formula::{FConj, FDisj, FNeg, FVar};

// Invariant: Every sub-formula is already flat and DNF
fn flatten(f: Formula) -> Formula {
    match f {
        FVar(_) => f,
        FNeg(fr) => match *fr {
            FVar(x) => FNeg(Box::new(FVar(x))),
            FNeg(f_bot) => *f_bot,
            FConj(_) => panic!("Cannot have Conj below Disj"),
            FDisj(_) => panic!("Cannot have Disj below Conj"),
        },
        FDisj(fvec) => {
            let mut ret_vec = Vec::new();
            for fv in fvec {
                match fv {
                    FVar(_) => ret_vec.push(fv),
                    FNeg(_) => ret_vec.push(fv),
                    FDisj(mut fvec_nested) => ret_vec.append(&mut fvec_nested),
                    FConj(mut fvec_nested) => ret_vec.append(&mut fvec_nested),
                }
            }
            FDisj(ret_vec)
        }
        FConj(fvec) => {
            let mut ret_vec = Vec::new();
            for fv in fvec {
                match fv {
                    FVar(_) => ret_vec.push(fv),
                    FNeg(_) => ret_vec.push(fv),
                    FDisj(_) => panic!("Should not have Disj below Conj"),
                    FConj(mut fvec_nested) => ret_vec.append(&mut fvec_nested),
                }
            }
            FConj(ret_vec)
        }
    }
}

// // Invariant: Every sub-formula is already flat and DNF
// fn flatten(f: &mut Formula) {
//     match f {
//         FVar(_) => (),
//         FNeg(f_new) => match **f_new {
//             FVar(_) => (),
//             FNeg(f_bot) => *f = *f_bot,
//             FConj(_) => panic!("Cannot have Conj below Disj"),
//             FDisj(_) => panic!("Cannot have Disj below Conj"),
//         },
//         FDisj(mut fvec) | FConj(mut fvec) => {
//             let mut new_vec = Vec::new();
//             for fv in fvec.into_iter() {
//                 match fv {
//                     FDisj(mut fvec_nested) | FConj(mut fvec_nested) => {
//                         new_vec.append(&mut fvec_nested);
//                     }
//                     _ => {}
//                 }
//             }
//             fvec = new_vec;
//             // *fvec = &new_vec;
//         }
//     }
// }

// Invariant: All formulas in fvec vector are in DNF form
fn formula_cross(fvec: Vec<Formula>) -> Formula {
    let mut ret_vec: Vec<Vec<Formula>> = vec![vec![]];
    for f in fvec {
        match f {
            FVar(_) | FNeg(_) => {
                ret_vec.par_iter_mut().for_each(|vec| {
                    vec.push(f.clone());
                });
                // for vec in &mut ret_vec {
                //     vec.push(f.clone())
                // }
            }
            FDisj(fvec_rec) => {
                let new_ret_vec: Arc<Mutex<Vec<Vec<Formula>>>> = Arc::new(Mutex::new(Vec::new()));
                ret_vec.par_iter().for_each(|vec| {
                    let mut cur_ret_vec = vec![];
                    for form in &fvec_rec {
                        let mut cur_vec = vec.clone();
                        cur_vec.push(form.clone());
                        cur_ret_vec.push(cur_vec);
                    }
                    new_ret_vec.lock().unwrap().extend(cur_ret_vec);
                });
                // let mut new_ret_vec = vec![];
                // for vec in ret_vec {
                //     for form in &fvec_rec {
                //         let mut cur_vec = vec.clone();
                //         cur_vec.push(form.clone());
                //         new_ret_vec.push(cur_vec);
                //     }
                // }
                ret_vec = new_ret_vec.lock().unwrap().clone();
            }
            FConj(fvec_rec) => {
                ret_vec
                    .par_iter_mut()
                    .for_each(|vec| vec.append(&mut fvec_rec.clone()));
                // for vec in &mut ret_vec {
                //     vec.append(&mut fvec_rec);
                // }
            }
        }
    }
    let final_vec: Vec<Formula> = ret_vec.into_iter().map(|vec| FConj(vec)).collect();
    FDisj(final_vec)
}

fn to_dnf(f: Formula) -> Formula {
    match f {
        FVar(_) => f,
        FDisj(fvec) => {
            // let mut ret_vec = Vec::new();
            // for ele in fvec.into_iter() {
            //     let rec_call = to_dnf(ele);
            //     ret_vec.push(rec_call)
            // }
            let ret_vec = fvec.into_par_iter().map(|ele| to_dnf(ele)).collect();
            flatten(FDisj(ret_vec))
        }
        FNeg(v) => match *v {
            FVar(x) => FNeg(Box::new(FVar(x))),
            FNeg(frr) => to_dnf(*frr),
            FConj(fvec) => {
                // let mut ret_vec = Vec::new();
                // for ele in fvec.into_iter() {
                //     let rec_call = to_dnf(FNeg(Box::new(ele)));
                //     ret_vec.push(rec_call)
                // }
                let ret_vec = fvec
                    .into_par_iter()
                    .map(|ele| to_dnf(FNeg(Box::new(ele))))
                    .collect();
                to_dnf(flatten(FDisj(ret_vec)))
            }
            FDisj(fvec) => {
                // let mut ret_vec = Vec::new();
                // for ele in fvec.into_iter() {
                //     let rec_call = to_dnf(FNeg(Box::new(ele)));
                //     ret_vec.push(rec_call)
                // }
                let ret_vec = fvec
                    .into_par_iter()
                    .map(|ele| to_dnf(FNeg(Box::new(ele))))
                    .collect();
                to_dnf(flatten(FConj(ret_vec)))
            }
        },
        FConj(fvec) => {
            // let mut ret_vec = Vec::new();
            // for ele in fvec.into_iter() {
            //     let rec_call = to_dnf(ele);
            //     ret_vec.push(rec_call);
            // }
            let ret_vec: Vec<Formula> = fvec.into_par_iter().map(|ele| to_dnf(ele)).collect();
            formula_cross(ret_vec)
        }
    }
}

fn proc_line(line: Result<String, std::io::Error>) -> Formula {
    // println!("{}", line);
    let good_line = line.expect("String needed").trim().replace("  ", " ");
    let vars = good_line.split(" ");
    let mut ret_val: Vec<Formula> = Vec::new();
    for ele in vars {
        let val: i32 = ele
            .parse()
            .expect(&format!("CRITICAL ERROR PARSING LINE: '{good_line}'"));
        if val == 0 {
            // 0 is the last element, so done
            break;
        }
        if val > 0 {
            let val_good = val as u32;
            ret_val.push(FVar(val_good));
        } else {
            let val_good = -val as u32;
            ret_val.push(FNeg(Box::new(FVar(val_good))));
        }
    }
    FDisj(ret_val)
}

fn disj_below_neg(f: Formula, in_neg: bool) -> bool {
    match f {
        FVar(_) => false,
        FNeg(frr) => disj_below_neg(*frr, true),
        FDisj(rec_vec) => {
            if in_neg {
                return true;
            }
            for val in rec_vec {
                if disj_below_neg(val, in_neg) {
                    return true;
                }
            }
            false
        }
        FConj(rec_vec) => {
            for val in rec_vec {
                if disj_below_neg(val, in_neg) {
                    return true;
                }
            }
            false
        }
    }
}

fn disj_below_conj(f: Formula, in_conj: bool) -> bool {
    match f {
        FVar(_) => false,
        FNeg(frr) => disj_below_neg(*frr, in_conj),
        FDisj(rec_vec) => {
            if in_conj {
                return true;
            }
            for val in rec_vec {
                if disj_below_neg(val, in_conj) {
                    return true;
                }
            }
            false
        }
        FConj(rec_vec) => {
            for val in rec_vec {
                if disj_below_neg(val, true) {
                    return true;
                }
            }
            false
        }
    }
}

fn dnf_spec(f: Formula, in_neg: bool, in_conj: bool, in_disj: bool) -> bool {
    match f {
        FVar(_) => true,
        FNeg(frr) => dnf_spec(*frr, true, in_conj, in_disj),
        FDisj(ret_vec) => {
            if in_neg || in_conj || in_disj {
                return false;
            }
            for vec in ret_vec {
                if !dnf_spec(vec, in_neg, in_conj, true) {
                    return false;
                }
            }
            true
        }
        FConj(ret_vec) => {
            if in_neg || in_conj {
                return false;
            }
            for vec in ret_vec {
                if !dnf_spec(vec, in_neg, true, in_disj) {
                    return false;
                }
            }
            true
        }
    }
}

/**
 * Returns true if val \in v, false if val \not\in v
 * Always makes sure that after execution, -val \in v is true
 *
 * If safe_insert_vec returns FALSE => UNSAT
 * If safe_insert_vec returns TRUE => SAT and val \in v now
 */
fn safe_insert_vec(v: &mut Vec<i64>, val: i64) -> bool {
    if v.contains(&-val) {
        return false;
    }
    v.push(val);
    true
}

fn sat_conj(f: Formula) -> bool {
    let mut count_vec = Vec::new();
    match f {
        FVar(_) => panic!("Var inside conj"),
        FNeg(_) => panic!("Neg inside conj"),
        FDisj(_) => panic!("Non-flat DISJ"),
        FConj(vec) => {
            for v in vec {
                match v {
                    FVar(x) => {
                        let xi: i64 = x.into();
                        if !safe_insert_vec(&mut count_vec, xi) {
                            return false;
                        }
                    }
                    FNeg(frr) => match *frr {
                        FVar(x) => {
                            let xi: i64 = (x).into();
                            let x_neg = -xi;
                            if !safe_insert_vec(&mut count_vec, x_neg) {
                                return false;
                            }
                        }
                        FNeg(_) => panic!("Neg in Neg in Conj"),
                        FDisj(_) => panic!("Disj in Neg in Conj"),
                        FConj(_) => panic!("Conj in Neg in Conj"),
                    },
                    FDisj(_) => panic!("Disj inside conj"),
                    FConj(_) => panic!("Non flat conj"),
                }
            }
            true
        }
    }
}

fn sat(f: Formula) -> bool {
    let solution_found = AtomicBool::new(false);

    match f {
        FVar(_) => panic!("Top level var"),
        FNeg(_) => panic!("Top level neg"),
        FDisj(ret_vec) => ret_vec.into_par_iter().any(|vec| {
            if solution_found.load(std::sync::atomic::Ordering::Relaxed) {
                return true;
            } else {
                let res = sat_conj(vec);
                if res {
                    solution_found.store(true, std::sync::atomic::Ordering::Relaxed);
                }
                res
            }
        }),
        FConj(_) => panic!("Why top level conj!"),
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() != 2 {
        eprintln!("Usage: {} <input_file>", args[0]);
        std::process::exit(-1);
    }

    let file_name = &args[1];

    let file = File::open(file_name).expect("Failed to open file!");

    let reader = BufReader::new(file);

    let mut lines = reader.lines();
    println!("READ LINES INTO BUFREADER");
    // READ THE FIRST INFO LINE
    lines.next();

    let ret_form_vec: Vec<Formula> = lines.into_iter().map(|line| proc_line(line)).collect();
    // par_bridge().map(|line| proc_line(line)).collect();
    println!("Welcome to Converter");
    let formula = to_dnf(FConj(ret_form_vec));
    // println!(
    //     "DONE: DISJ_BELOW_CONJ = {}; DISJ_BELOW_NEG = {}; DNF_SPEC = {}",
    //     disj_below_neg(formula.clone(), false),
    //     disj_below_conj(formula.clone(), false),
    //     dnf_spec(formula.clone(), false, false, false)
    // );

    println!("Checking SAT");
    let sat = sat(formula);
    println!("SAT: {sat}")
}
