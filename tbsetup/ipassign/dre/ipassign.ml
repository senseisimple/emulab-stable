(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *)

(*
 * ipassign.ml - functions common to many ipassign tasks
 *)

let special_args = ["vertex-count"; "edge-count"; "first-hops"; "dre";
                    "eigs"; "eig2"; "name"];;

(*
 * Globals set by command-line args
 *)
let (vertex_count : int option ref) = ref None;;
let (edge_count : int option ref) = ref None;;
let (first_hop_file : string option ref) = ref None;;
let (dre_file : string option ref) = ref None;;
let (eigs_file : string option ref) = ref None;;
let (eig2_file : string option ref) = ref None;;
let (name_file : string option ref) = ref None;;

(*
 * Information read in from files, etc.
 *)
let (first_hops : int array array option ref) = ref None;;

(*
 * Process command-line arguments that are common to many/all ipassign ocaml
 * programs
 *)
let processCommonArgs (argv : string array) : string array =
    let arglist = Array.to_list argv in
    let filter_function (elt : string) : bool =
        let check_for_specials (check_against : string) (sofar : bool)
                               (special : string) = 
            sofar &&  Str.string_match
                        (Str.regexp ("^--" ^ special ^ "=.*")) check_against 0
        in
        List.fold_left (check_for_specials elt) false special_args
    in
    let (myargs,notmine) = List.partition filter_function arglist in
    (* Do something with myargs *)
    Array.of_list notmine
;;

exception ArgNotGiven of string;;
let returnFromFileOrDie (var : 'a option ref) (filename : string option)
                        (descr : string) (reader : in_channel -> 'a) : 'a =
    match !var with
       Some(data) -> data
    |  None ->
            match filename with
               None -> raise (ArgNotGiven descr)
            |  Some(fname) -> begin
                let channel = open_in fname in
                let data = reader channel in
                var := Some(data);
                data
               end

;;

let getFH () : int array array = 
    returnFromFileOrDie first_hops !first_hop_file "First Hop File"
        (fun x -> [| [| |] |])
;;
