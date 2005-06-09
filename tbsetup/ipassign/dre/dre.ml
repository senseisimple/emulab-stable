(*
 * Functions for computing DRE
 *)

let pairwise_dre (hops : ('a, 'b) Dijkstra.first_hop array array)
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
            if Dijkstra.fh_equal hops.(w).(i) hops.(w).(j) then
                num_equiv := !num_equiv + 1;
            samples_taken := !samples_taken + 1
        end
    done;
    (* (float_of_int !num_equiv) *)
    ((float_of_int !num_equiv) /. (float_of_int !samples_taken))
;;

(* let compute_dre (graph : ('a, 'b) Graph.t) : float array array = *)
let compute_dre ?(samples=None)
        (hops : ('a, 'b) Dijkstra.first_hop array array) : float array array =
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
