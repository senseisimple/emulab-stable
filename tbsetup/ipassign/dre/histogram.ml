(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *)

(*
 * Histogram utility functions
 *)

type elt = { min : float; count : int };;

type t = elt list;;

let rec print_histogram (hist : t) : unit =
    match hist with
      [] -> ()
    | x::xs -> (Printf.printf "%0.4f\t%i\n" x.min x.count; print_histogram xs)
;;

let rec empty_histogram (l : float list) : t =
    match l with
      [] -> []
    | x::xs -> { min = x; count = 0 } :: empty_histogram xs
;;
(* empty_histogram [1.0; 0.5; 0.0];; *)

let rec insert_item (elt : float) (l : t) : t =
    match l with
      [] -> raise (Failure "No place in list for elt")
    | x::xs when elt >= x.min ->
            { min = x.min; count = (x.count + 1) } :: xs
    | x::xs -> x :: insert_item elt xs
;;
(*
insert_item 0.75 (empty_histogram [1.0; 0.5; 0.0]);;
insert_item 0.0 (empty_histogram [1.0; 0.5; 0.0]);;
insert_item 1.0 (empty_histogram [1.0; 0.5; 0.0]);;
*)

let rec fixed_size_steps (low : float) (step : float) (steps : int) : float list =
    if steps = 0 then
        []
    else
        low :: fixed_size_steps (low +. step) step (steps - 1)
;;
(*
fixed_size_steps 0.0 0.1 10;;
fixed_size_steps 0.5 0.25 3;;
*)

let steps_in_range (low : float) (high: float) (steps : int) : float list =
    let step_size = (low +. high) /. (float_of_int (steps -1)) -. low in
    fixed_size_steps low step_size steps
;;
(*
steps_in_range 0.0 1.0 11;;
steps_in_range 0.5 1.0 3;;
*)

let make_histogram (l : float list) (bins : float list) : t =
    let sbins = List.fast_sort (fun x y -> compare y x) bins in
    let rec insert_helper (elts : float list) (accum : t) : t = 
        match elts with
          [] -> accum
        | x::xs -> insert_helper xs (insert_item x accum) in
    insert_helper l (empty_histogram sbins)
;;
(*
make_histogram [1.0; 0.75; 0.5; 0.25; 0.0] [0.5; 0.0];;
make_histogram [1.0; 0.75; 0.5; 0.25; 0.0] [0.0; 1.0];;
print_histogram (make_histogram [1.0; 0.75; 0.5; 0.25; 0.0] [0.5; 0.0]);;
*)
