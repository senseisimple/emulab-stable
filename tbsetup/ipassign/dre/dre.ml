(*
 * Functions for computing DRE
 *)

module ISet = Set.Make(struct type t = int let compare = compare end);;
type nodeset = ISet.t;;

let pairwise_dre (hops : 'a array array)
        (i : int) (j : int) (samples : int option) : float =
    let num_equiv = ref 0 in
    let samples_taken = ref 0 in
    let nsamples = match samples with
                     None -> (Array.length hops) - 1
                   | Some(x) -> x in
    for k = 0 to nsamples do
        let w = match samples with
                  None -> k
                | Some(x) -> Random.int (((Array.length hops) - 1)) in
        (*if (w != i && w != j) then*) begin
            if hops.(w).(i) = hops.(w).(j) then
                num_equiv := !num_equiv + 1;
            samples_taken := !samples_taken + 1
        end
    done;
    (* (float_of_int !num_equiv) *)
    ((float_of_int !num_equiv) /. (float_of_int !samples_taken))
;;

(* let compute_dre (graph : ('a, 'b) Graph.t) : float array array = *)
let compute_dre ?(samples=None) (hops : 'a array array) : float array array =
    (* Initialize the array of results *)
    let n = Array.length hops in
    let dre_matrix = Array.make_matrix n n 42.0 in
    (* For now, try the simple, naive, way *)
    for i = 0 to (n - 1) do
        (* for j = (i+1) to (n - 1) do *)
        for j = 0 to (n - 1) do
            (* Time optimization - TODO - do the space optimizatio
            * at some point too *)
            (* if i < j then dre_matrix.(i).(j) <- pairwise_dre hops i j samples *)
            dre_matrix.(i).(j) <- pairwise_dre hops i j samples
        done
    done;
    dre_matrix
;;

let list_of_dre_matrix (matrix : float array array) : float list =
    let n = Array.length matrix in
    let lref = ref [] in
    for i = 0 to (n - 1) do
        for j = 0 to (n - 1) do
            if i < j then
                lref := matrix.(i).(j) :: !lref
        done
    done;
    !lref
;;

let find_min_max (matrix : float array array) (n : int) : (float * float) =
    let min = ref 43.0 in
    let max = ref 0.0 in
    for i = 0 to ((Array.length matrix) - 1) do
        if i != n then begin
            let elt = (if i > n then matrix.(n).(i) else matrix.(i).(n)) in
            if elt < !min then min := elt;
            if elt > !max then max := elt
        end
    done;
    (!min,!max)
;;

let score_ordering (matrix : float array array) (ordering : int array)
        : float =
    (* print_endline "New score"; *)
    let size = Array.length ordering in
    let pairwise_score (a : int) (b : int) (x : int) (y : int) : float =
        (*
        print_endline ("Pairwise between " ^ (string_of_int a) ^ " and " ^ (string_of_int b));
        print_endline ("Nodes are " ^ (string_of_int x) ^ " and " ^ (string_of_int y));
        print_endline ("Distance: " ^ (string_of_int (b - a)));
        print_endline ("Matrix: " ^ (string_of_float matrix.(x).(y)));
        *)
        (* (float_of_int (b - a)) (* *. (float_of_int (b - a)) *) *.  matrix.(x).(y) *)
        (float_of_int (b - a)) (* /. (float_of_int size) *)  *.  matrix.(x).(y)
    in
    let score = ref 0.0 in
    for i = 0 to (size - 1) do
        for j = (i + 1) to (size - 1) do
            score := !score +. (pairwise_score i j ordering.(i) ordering.(j))
            (* print_endline ("Score " ^ (string_of_float !score)) *)
        done
    done; 
    (* print_endline ("Arrived at score " ^ (string_of_float !score)); *)
    !score
;;

(* Not a great place to put this *)
type blob = ISet.t;;

(*
 * Find the RES set for two nodes
 *)
let pairwise_res (hops : 'a array array) (i : int) (j : int)
        : nodeset  =
    let rec helper (k : int) : nodeset =
        if k < 0 then
            ISet.empty
        else
            if hops.(k).(i) = hops.(k).(j) then
                ISet.add k (helper (k - 1))
            else
                helper (k - 1)
    in
    helper ((Array.length hops) - 1)
;;

(*
 * Find the RES set for a set of nodes
 *)
let setwise_res (hops : 'a array array) (s : nodeset)
        : nodeset  =
    (* Chose a representative node from the set to compare first hops with *)
    let representative = ISet.choose s in
    let rec eval_node (k : int) : nodeset =
        if k < 0 then
            ISet.empty
        else
            (*
            let all_hops_equal (node : int) (accum : bool) : bool =
                (* TODO Could make this faster by not using fold, so that we can
                 * short-circuit *)
                accum && (hops.(k).(node) = hops.(k).(representative)) in
            if ISet.fold all_hops_equal s true then
            *)
            if ISet.for_all (fun node -> hops.(k).(node) =
                hops.(k).(representative)) s then
                (* Add this node to the set we're creating *)
                ISet.add k (eval_node (k - 1))
            else
                (* Nope, just check the next node *)
                eval_node (k - 1)
    in
    eval_node ((Array.length hops) - 1)
;;

(*
 * Merge the RES sets for two sets of nodes
 *)
let merge_res (hops : 'a array array)
        (s1 : nodeset) (s2 : nodeset) (res_s1 : nodeset) (res_s2 : nodeset)
        : nodeset =
    (* TODO - This could be made faster by intersecting s1 and s2 first, then
     * only looking at the first hop table for the intersection *)
    (*
    let res_s1s2 = pairwise_res hops (ISet.choose s1) (ISet.choose s2) in
    ISet.inter res_s1 (ISet.inter res_s2 res_s1s2)
    *)
    (*
    let res_inter = ISet.inter res_s1 res_s2 in
    ISet.fold () res_inter ISet.empty
    *)
    (* For some reason I can't fathom, this is slower than setwise_res
    let r1 = ISet.choose s1 in
    let r2 = ISet.choose s2 in
    let res_inter = ISet.inter res_s1 res_s2 in
    ISet.fold (fun x sofar -> if hops.(x).(r1) = hops.(x).(r2) then ISet.add x sofar
    else sofar) res_inter ISet.empty
    *)
    setwise_res hops (ISet.union s1 s2)
;;


(*
 * Score a potential combination - bigger is better
 *)
let score_res (res_s : nodeset) : int =
    ISet.cardinal res_s
;;

