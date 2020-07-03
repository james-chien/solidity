{
    function _...($..) {}
    let a...
    _...(a...)
}
// ----
// TypeError 3384: (6-27): "_..." is not a valid identifier (ends with a dot).
// TypeError 7771: (6-27): "_..." is not a valid identifier (contains consecutive dots).
// TypeError 3384: (20-23): "$.." is not a valid identifier (ends with a dot).
// TypeError 7771: (20-23): "$.." is not a valid identifier (contains consecutive dots).
// TypeError 3384: (36-40): "a..." is not a valid identifier (ends with a dot).
// TypeError 7771: (36-40): "a..." is not a valid identifier (contains consecutive dots).
