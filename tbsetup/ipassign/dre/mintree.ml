(*
 * mintree.ml - Algorithms for finding the minimum-height tree from a forest
 * of trees.
 *)

(* Maximum depth of a tree *)
let max_depth = 32;;

(* A collection of bins *)
type bins_t = int array;;

(* Caller tried to remove from an empty bin *)
exception EmptyBin;;

(*
 * Add one to a given bin
 *)
let add_to_bin (bins : bins_t) (which : int) : bins_t =
    bins.(which) <- bins.(which) + 1;
    bins
;;

(*
 * Remove one from a given bin
 *)
let remove_from_bin (bins : bins_t) (which : int) : bins_t =
    if bins.(which) = 0 then raise EmptyBin;
    bins.(which) <- bins.(which) - 1;
    bins
;;

(*
 * Make a new set of bins
 *)
let make (size : int) : bins_t =
    Array.make size 0
;;

exception TooManyBits;;
(*
 * Find the height of the minimum-height tree from the given set of bins
 *)
let height_of (bins : bins_t) : int =
    (* Make a copy of the bins so we can modify them *)
    let newbins = Array.copy bins in
    let len = Array.length newbins in
    (* This will keep track of how many bit's we've had to use *)
    let bits_used = ref 0 in
    (* Loop for every bin *)
    for i = 0 to len - 1 do
        if bins.(i) != 0 then (
            let quotient = bins.(i) / 2 in
            let remainder = bins.(i) mod 2 in

            (* If we are on a bin that only has one member, then it's
             * possible we're done with the algorithm *)
            let are_done = ref true in
            if (quotient = 0) && (remainder = 1) then (
                (* Look for any later bins that have non-zero values *)
                for j = i + 1 to len - 1 do (
                    if bins.(j) != 0 then are_done := false;
                ) done;
            ) else (
                (* We can't be done unless we find a one-member bin *)
                are_done := false;
            );
            if !are_done then (
                (* Note, if we're done, we'll continue the outer loop all the
                 * way to the end of the bitspace, but we're guaranteed not to
                 * find anything *)
                bits_used := i
            ) else (
                if (quotient > 0) && (i = len - 1) then (
                    raise TooManyBits
                );
                (* Two trees of this height can be replaced with one of the next
                 * height *)
                bins.(i + 1) <- bins.(i + 1) + quotient;
                (* If there's a leftover, we 'promote' it to the next height,
                 * where it'll get combined with some larger subtree *)
                if (remainder != 0) then (
                    bins.(i + 1) <- bins.(i + 1) + remainder
                );
                (* Okay, done with this bin *)
                bins.(i) <- 0
            )
        ) else (
            (* We don't have to do anything to empty bins *)
            ()
        )
    done;
    (* Return the number of bits used *)
    !bits_used
;;

(*
 * Tests
 *)
(*
let mybins = make 32;;
add_to_bin mybins 1;;
add_to_bin mybins 1;;
add_to_bin mybins 1;;
add_to_bin mybins 4;;
add_to_bin mybins 4;;
add_to_bin mybins 4;;
print_endline (string_of_int (height_of  mybins));
*)
