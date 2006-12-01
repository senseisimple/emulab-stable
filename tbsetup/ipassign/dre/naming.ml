(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *)

(*
 * naming.ml
 * Definition of, and functions to read, a naming for a graph
 *)

type naming = int array;;
type ordering = int array;;

let read_naming_file (filename : string) : naming =
    let channel = open_in filename in
    (*
     * This is ridiculously inefficient, but to get much better, we'd need a
     * better file format
     *)
    let rec get_naming () : naming =
        try
            let line = input_line channel in
            let value = int_of_string line in
            Array.append [| value |] (get_naming ())
        with
            End_of_file -> [| |]
    in
    get_naming ()
;;

let naming_of_ordering(order : ordering) : naming =
    let names = Array.make (Array.length order) 0 in
    Array.iteri (fun ind cont -> names.(cont) <- ind) order;
    names
;;

let ordering_of_naming (names : naming) : ordering =
    (* Actually the same operation as naming_from_ordering *)
    naming_of_ordering names
;;

let print_naming (names : naming) : unit =
    Array.iter (fun x -> print_endline (string_of_int x)) names
;;
